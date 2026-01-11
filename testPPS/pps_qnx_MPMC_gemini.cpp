#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <cstring>
#include <sstream>

// --- Constants ---
const std::string PPS_ROOT = "/pps/myapp/";

// --- Helper: PPS Parser (Simplified) ---
// In production, use a proper PPS parser to handle object syntax (key::value)
std::string extractValue(const std::string& rawData, const std::string& key) {
    auto pos = rawData.find(key + "::");
    if (pos == std::string::npos) return "";
    auto start = pos + key.length() + 2;
    auto end = rawData.find('\n', start);
    return rawData.substr(start, end - start);
}

// ==========================================
// 1. The Reactor (Event Loop Manager)
// ==========================================
// This class manages the select() loop so Views don't need their own threads.
class PPSReactor {
public:
    using Callback = std::function<void(int fd)>;

    static PPSReactor& instance() {
        static PPSReactor instance;
        return instance;
    }

    // Register a file descriptor and the callback to run when data is ready
    void registerFD(int fd, Callback cb) {
        std::lock_guard<std::mutex> lock(mtx_);
        callbacks_[fd] = cb;
        if (fd > max_fd_) max_fd_ = fd;
        // Interrupt the select loop to update sets (omitted for brevity, usually done via pipe)
    }

    void unregisterFD(int fd) {
        std::lock_guard<std::mutex> lock(mtx_);
        callbacks_.erase(fd);
        close(fd);
    }

    // The main loop running in a separate thread
    void start() {
        running_ = true;
        worker_ = std::thread([this]() {
            fd_set read_fds;
            struct timeval timeout;

            while (running_) {
                FD_ZERO(&read_fds);
                int current_max = 0;

                {
                    std::lock_guard<std::mutex> lock(mtx_);
                    if (callbacks_.empty()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    }
                    for (const auto& pair : callbacks_) {
                        FD_SET(pair.first, &read_fds);
                        if (pair.first > current_max) current_max = pair.first;
                    }
                }

                timeout.tv_sec = 1; 
                timeout.tv_usec = 0;

                // select() waits here efficiently
                int activity = select(current_max + 1, &read_fds, nullptr, nullptr, &timeout);

                if (activity > 0) {
                    std::lock_guard<std::mutex> lock(mtx_);
                    for (auto& pair : callbacks_) {
                        int fd = pair.first;
                        if (FD_ISSET(fd, &read_fds)) {
                            // Trigger the View's callback
                            pair.second(fd);
                        }
                    }
                }
            }
        });
    }

    void stop() {
        running_ = false;
        if (worker_.joinable()) worker_.join();
    }

private:
    PPSReactor() : running_(false), max_fd_(0) {}
    ~PPSReactor() { stop(); }

    std::unordered_map<int, Callback> callbacks_;
    std::mutex mtx_;
    std::thread worker_;
    std::atomic<bool> running_;
    int max_fd_;
};

// ==========================================
// 2. The Model (Publisher)
// ==========================================
class Model {
public:
    Model(const std::string& objectName) {
        path_ = PPS_ROOT + objectName;
        // Ensure directory exists (simplified)
        mkdir("/pps/myapp", 0777); 
        
        // Open for writing. O_CREAT needed if it doesn't exist.
        fd_ = open(path_.c_str(), O_WRONLY | O_CREAT, 0666);
        if (fd_ < 0) std::cerr << "Error opening model: " << path_ << std::endl;
    }

    ~Model() { if (fd_ >= 0) close(fd_); }

    void updateData(const std::string& key, const std::string& value) {
        if (fd_ < 0) return;
        
        // PPS Format: key::value\n
        std::string payload = key + "::" + value + "\n";
        write(fd_, payload.c_str(), payload.size());
    }

private:
    std::string path_;
    int fd_;
};

// ==========================================
// 3. The View (Subscriber)
// ==========================================
class View {
public:
    View(const std::string& objectName, const std::string& id) : id_(id) {
        // IMPORTANT: Open with ?wait,delta
        // ?wait: keeps the connection alive
        // ?delta: only returns changes
        std::string path = PPS_ROOT + objectName + "?wait,delta";
        
        fd_ = open(path.c_str(), O_RDONLY | O_CREAT, 0666);
        if (fd_ < 0) {
            std::cerr << "View " << id_ << " failed to open PPS." << std::endl;
            return;
        }

        // Register myself with the Reactor
        PPSReactor::instance().registerFD(fd_, [this](int fd) {
            this->onDataReady(fd);
        });
    }

    ~View() {
        PPSReactor::instance().unregisterFD(fd_);
    }

private:
    // Called by the Reactor thread when select() detects data
    void onDataReady(int fd) {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        
        // Non-blocking read is safer here, but standard read is fine if select said yes
        ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            std::string data(buffer);
            // Parse and update UI
            std::cout << "[View " << id_ << "] Received Update: " << data;
            
            // Example: Parse a specific key
            std::string rpm = extractValue(data, "rpm");
            if (!rpm.empty()) {
                std::cout << "[View " << id_ << "] RPM updated to: " << rpm << std::endl;
            }
        }
    }

    std::string id_;
    int fd_;
};

// ==========================================
// 4. Main Application
// ==========================================
int main() {
    // 1. Start the IO Reactor
    PPSReactor::instance().start();

    // 2. Create Models (Publishers)
    Model engineModel("engine");
    Model mediaModel("media");

    // 3. Create Views (Subscribers)
    // Notice: We don't need manual threads here. The Reactor handles them.
    View clusterView("engine", "Cluster");
    View hudView("engine", "HUD");
    View radioView("media", "RadioDisplay");

    // 4. Simulate System Running
    std::cout << "--- System Started ---" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "\n> Engine RPM increasing..." << std::endl;
    engineModel.updateData("rpm", "2500");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "\n> Changing Song..." << std::endl;
    mediaModel.updateData("track", "Bohemian_Rhapsody");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "\n> Engine RPM Redline..." << std::endl;
    engineModel.updateData("rpm", "6000");

    // Cleanup
    std::this_thread::sleep_for(std::chrono::seconds(2));
    PPSReactor::instance().stop();
    return 0;
}