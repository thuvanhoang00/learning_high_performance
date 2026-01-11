#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>
#include <sys/iofunc.h>
#include <pps.h>

namespace PPS {

class PPSMessage {
public:
    std::string objectName;
    std::string data;
    uint64_t timestamp;
    
    PPSMessage(std::string name, std::string data, uint64_t ts)
        : objectName(std::move(name)), data(std::move(data)), timestamp(ts) {}
};

class PPSReader {
private:
    std::atomic<bool> running{true};
    std::thread readerThread;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::queue<PPSMessage> messageQueue;
    std::unordered_map<std::string, int> fdMap;
    std::vector<int> fdList;
    fd_set masterSet;
    int maxFd = -1;

    void addFd(int fd) {
        FD_SET(fd, &masterSet);
        if (fd > maxFd) maxFd = fd;
    }

    void removeFd(int fd) {
        FD_CLR(fd, &masterSet);
        if (fd == maxFd) {
            while (maxFd >= 0 && !FD_ISSET(maxFd, &masterSet)) maxFd--;
        }
    }

public:
    PPSReader() {
        FD_ZERO(&masterSet);
    }

    ~PPSReader() {
        stop();
    }

    bool subscribe(const std::string& objectPath) {
        int fd = open(objectPath.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) return false;
        
        fdMap[objectPath] = fd;
        fdList.push_back(fd);
        addFd(fd);
        
        // Initial read to get current state
        char buffer[4096];
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.emplace(objectPath, std::string(buffer), getTimestamp());
            cv.notify_one();
        }
        
        return true;
    }

    void processMessages(std::function<void(const PPSMessage&)> handler) {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!messageQueue.empty()) {
            handler(messageQueue.front());
            messageQueue.pop();
        }
    }

    void start() {
        readerThread = std::thread([this] {
            while (running) {
                fd_set readSet = masterSet;
                struct timeval timeout;
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                
                int ready = select(maxFd + 1, &readSet, nullptr, nullptr, &timeout);
                
                if (ready < 0) {
                    if (errno == EINTR) continue;
                    break;
                }
                
                if (ready == 0) continue; // timeout
                
                for (int fd : fdList) {
                    if (FD_ISSET(fd, &readSet)) {
                        char buffer[4096];
                        ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
                        if (bytesRead > 0) {
                            buffer[bytesRead] = '\0';
                            std::string objectPath;
                            for (const auto& pair : fdMap) {
                                if (pair.second == fd) {
                                    objectPath = pair.first;
                                    break;
                                }
                            }
                            
                            {
                                std::lock_guard<std::mutex> lock(queueMutex);
                                messageQueue.emplace(objectPath, std::string(buffer), getTimestamp());
                                cv.notify_one();
                            }
                        }
                    }
                }
            }
        });
    }

    void stop() {
        running = false;
        if (readerThread.joinable()) {
            readerThread.join();
        }
        
        for (auto& pair : fdMap) {
            close(pair.second);
        }
    }

    uint64_t getTimestamp() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1000000000ULL + ts.ts_nsec;
    }
};

class PPSWriter {
private:
    std::unordered_map<std::string, int> fdMap;
    std::mutex writeMutex;

public:
    ~PPSWriter() {
        for (auto& pair : fdMap) {
            close(pair.second);
        }
    }

    bool publish(const std::string& objectPath, const std::string& data) {
        std::lock_guard<std::mutex> lock(writeMutex);
        
        auto it = fdMap.find(objectPath);
        if (it == fdMap.end()) {
            int fd = open(objectPath.c_str(), O_WRONLY | O_CREAT, 0644);
            if (fd < 0) return false;
            fdMap[objectPath] = fd;
            it = fdMap.find(objectPath);
        }
        
        int fd = it->second;
        ssize_t written = write(fd, data.c_str(), data.size());
        return (written == static_cast<ssize_t>(data.size()));
    }
};

class PPSManager {
private:
    PPSReader reader;
    PPSWriter writer;
    std::thread processingThread;
    std::atomic<bool> running{true};
    std::mutex subscribersMutex;
    std::unordered_map<std::string, std::vector<std::function<void(const PPSMessage&)>>> subscribers;

    void processingLoop() {
        while (running) {
            reader.processMessages([this](const PPSMessage& msg) {
                std::lock_guard<std::mutex> lock(subscribersMutex);
                auto it = subscribers.find(msg.objectName);
                if (it != subscribers.end()) {
                    for (auto& handler : it->second) {
                        handler(msg);
                    }
                }
            });
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

public:
    PPSManager() {
        reader.start();
        processingThread = std::thread(&PPSManager::processingLoop, this);
    }

    ~PPSManager() {
        running = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
        reader.stop();
    }

    bool subscribe(const std::string& objectPath, std::function<void(const PPSMessage&)> handler) {
        if (!reader.subscribe(objectPath)) return false;
        
        std::lock_guard<std::mutex> lock(subscribersMutex);
        subscribers[objectPath].push_back(std::move(handler));
        return true;
    }

    bool publish(const std::string& objectPath, const std::string& data) {
        return writer.publish(objectPath, data);
    }
};

} // namespace PPS

// Usage example for Approach 1
class Model {
private:
    PPS::PPSManager& manager;
    std::string objectPath;
    
public:
    Model(PPS::PPSManager& mgr, const std::string& path) 
        : manager(mgr), objectPath(path) {}
    
    void updateData(const std::string& newData) {
        manager.publish(objectPath, newData);
    }
};

class View {
private:
    std::string modelName;
    
public:
    View(const std::string& name) : modelName(name) {}
    
    void onDataReceived(const PPS::PPSMessage& msg) {
        std::cout << "View " << modelName << " received: " << msg.data 
                  << " at " << msg.timestamp << std::endl;
    }
};

int main() {
    PPS::PPSManager manager;
    
    // Create models and views
    Model sensorModel(manager, "/pps/sensors/temperature");
    Model statusModel(manager, "/pps/system/status");
    
    View tempView("Temperature");
    View statusView("SystemStatus");
    
    // Subscribe views to models
    manager.subscribe("/pps/sensors/temperature", 
        [&tempView](const PPS::PPSMessage& msg) { tempView.onDataReceived(msg); });
    
    manager.subscribe("/pps/system/status",
        [&statusView](const PPS::PPSMessage& msg) { statusView.onDataReceived(msg); });
    
    // Simulate data updates
    while (true) {
        sensorModel.updateData("{\"temp\": 25.5, \"unit\": \"C\"}");
        statusModel.updateData("{\"status\": \"OK\", \"load\": 0.42}");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}