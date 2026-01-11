#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <functional>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <cstring>

// --- Helper Types ---
struct PPSMessage {
    std::string objectPath;
    std::string key;
    std::string value;
};

// Callback for Views: (Object Path, Data Payload)
using DataCallback = std::function<void(const std::string&, const std::string&)>;

// ==========================================
// 1. PPSWriter (Threaded "Fire & Forget")
// ==========================================
class PPSWriter {
public:
    PPSWriter() : running_(false) {}
    ~PPSWriter() { stop(); }

    void start() {
        running_ = true;
        worker_ = std::thread(&PPSWriter::processQueue, this);
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            running_ = false;
        }
        cv_.notify_one();
        if (worker_.joinable()) worker_.join();
    }

    // Main thread calls this (Thread-safe)
    void push(const std::string& path, const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mtx_);
        writeQueue_.push({path, key, value});
        cv_.notify_one();
    }

private:
    void processQueue() {
        while (true) {
            PPSMessage msg;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [this] { return !writeQueue_.empty() || !running_; });

                if (!running_ && writeQueue_.empty()) break;

                msg = writeQueue_.front();
                writeQueue_.pop();
            }

            // Perform the actual Write I/O here
            doWrite(msg);
        }
    }

    void doWrite(const PPSMessage& msg) {
        // Open/Write/Close for statelessness, or cache FDs for performance.
        // For Approach 1, caching FDs is better, but open/close is safer for demo.
        int fd = open(msg.objectPath.c_str(), O_WRONLY | O_CREAT, 0666);
        if (fd >= 0) {
            std::string payload = msg.key + "::" + msg.value + "\n";
            write(fd, payload.c_str(), payload.size());
            close(fd);
            // debug
            // std::cout << "[Writer] Wrote " << msg.key << " to " << msg.objectPath << std::endl;
        } else {
            std::cerr << "[Writer] Error opening " << msg.objectPath << std::endl;
        }
    }

    std::queue<PPSMessage> writeQueue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::thread worker_;
    bool running_;
};

// ==========================================
// 2. PPSReader (Single Thread, Multiple Files)
// ==========================================
class PPSReader {
public:
    PPSReader() : running_(false), max_fd_(0) {}
    ~PPSReader() { stop(); }

    void start() {
        running_ = true;
        worker_ = std::thread(&PPSReader::loop, this);
    }

    void stop() {
        running_ = false;
        if (worker_.joinable()) worker_.join();
        // Close all monitored FDs
        for (auto& pair : monitoredFiles_) {
            close(pair.first);
        }
    }

    // Register a file to watch and a callback to trigger
    void watch(const std::string& path, DataCallback cb) {
        // Must open with ?wait,delta to work with select() properly
        std::string ppsMode = path + "?wait,delta";
        int fd = open(ppsMode.c_str(), O_RDONLY | O_CREAT, 0666);
        
        if (fd >= 0) {
            std::lock_guard<std::mutex> lock(mtx_);
            monitoredFiles_[fd] = {path, cb}; // Store path and callback
            if (fd > max_fd_) max_fd_ = fd;
        }
    }

private:
    struct WatchInfo {
        std::string path;
        DataCallback callback;
    };

    void loop() {
        fd_set read_fds;
        struct timeval timeout;

        while (running_) {
            FD_ZERO(&read_fds);
            int current_max = 0;

            // Rebuild FD set
            {
                std::lock_guard<std::mutex> lock(mtx_);
                if (monitoredFiles_.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                for (auto const& [fd, info] : monitoredFiles_) {
                    FD_SET(fd, &read_fds);
                    if (fd > current_max) current_max = fd;
                }
            }

            timeout.tv_sec = 1; timeout.tv_usec = 0;
            
            // Wait for data
            int activity = select(current_max + 1, &read_fds, NULL, NULL, &timeout);

            if (activity > 0) {
                std::lock_guard<std::mutex> lock(mtx_);
                for (auto& [fd, info] : monitoredFiles_) {
                    if (FD_ISSET(fd, &read_fds)) {
                        char buffer[512];
                        int n = read(fd, buffer, sizeof(buffer)-1);
                        if (n > 0) {
                            buffer[n] = '\0';
                            // Dispatch to the specific listener
                            info.callback(info.path, std::string(buffer));
                        }
                    }
                }
            }
        }
    }

    std::unordered_map<int, WatchInfo> monitoredFiles_;
    std::mutex mtx_;
    std::thread worker_;
    int max_fd_;
    bool running_;
};

// ==========================================
// 3. The Manager (Facade)
// ==========================================
class PPSManager {
public:
    static PPSManager& instance() {
        static PPSManager inst;
        return inst;
    }

    void init() {
        writer_.start();
        reader_.start();
    }

    void shutdown() {
        writer_.stop();
        reader_.stop();
    }

    // Methods for Models (Publishers)
    void publish(const std::string& obj, const std::string& key, const std::string& val) {
        writer_.push("/pps/myapp/" + obj, key, val);
    }

    // Methods for Views (Subscribers)
    void subscribe(const std::string& obj, DataCallback cb) {
        reader_.watch("/pps/myapp/" + obj, cb);
    }

private:
    PPSManager() = default;
    PPSWriter writer_;
    PPSReader reader_;
};

// ==========================================
// 4. Usage Example
// ==========================================

// A Dummy View Class
class SpeedometerView {
public:
    SpeedometerView() {
        // Subscribe to the "sensors" object
        PPSManager::instance().subscribe("sensors", 
            [this](const std::string& path, const std::string& data) {
                this->onUpdate(data);
            });
    }

    void onUpdate(const std::string& data) {
        // In reality, parse "speed::100"
        std::cout << "[SpeedometerView] Drawing New Speed from data: " << data;
    }
};

int main() {
    // 1. Initialize the Central Manager
    PPSManager::instance().init();

    // 2. Create a View (it auto-subscribes in constructor)
    SpeedometerView speedo;

    std::cout << "--- Logic Running ---" << std::endl;

    // 3. Simulate a Model writing data (Main thread does NOT block here)
    // The writer thread handles the file I/O in the background
    PPSManager::instance().publish("sensors", "speed", "60");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    PPSManager::instance().publish("sensors", "speed", "80");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    PPSManager::instance().publish("sensors", "speed", "120");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    PPSManager::instance().shutdown();
    return 0;
}