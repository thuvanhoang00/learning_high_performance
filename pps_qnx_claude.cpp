#include <fcntl.h>
#include <sys/select.h>
#include <sys/pps.h>
#include <errno.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ============================================================================
// APPROACH 2: Multiple Independent Objects with select()
// ============================================================================

/**
 * A self-contained PPS publisher that manages its own file descriptor.
 * Each model can have its own PPSPublisher instance.
 */
class PPSPublisher {
public:
    explicit PPSPublisher(const std::string& pps_path) 
        : pps_path_(pps_path), fd_(-1) {
        open();
    }
    
    ~PPSPublisher() {
        close();
    }
    
    // Disable copy, enable move
    PPSPublisher(const PPSPublisher&) = delete;
    PPSPublisher& operator=(const PPSPublisher&) = delete;
    PPSPublisher(PPSPublisher&& other) noexcept
        : pps_path_(std::move(other.pps_path_)), fd_(other.fd_) {
        other.fd_ = -1;
    }
    
    /**
     * Publish data to PPS. This is synchronous and thread-safe.
     */
    bool publish(const std::string& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (fd_ < 0 && !open()) {
            return false;
        }
        
        // Write the PPS data
        ssize_t bytes = write(fd_, data.c_str(), data.length());
        
        if (bytes < 0) {
            // On error, try to reopen
            close();
            return false;
        }
        
        return true;
    }
    
    /**
     * Publish structured data using PPS attribute format.
     */
    bool publishAttribute(const std::string& name, const std::string& value, 
                         const std::string& type = "s") {
        // Format: attribute:type:value
        std::string data = name + ":" + type + ":" + value + "\n";
        return publish(data);
    }
    
    const std::string& getPath() const { return pps_path_; }
    
private:
    bool open() {
        if (fd_ >= 0) {
            return true;
        }
        
        fd_ = ::open(pps_path_.c_str(), O_WRONLY | O_CREAT, 0666);
        return fd_ >= 0;
    }
    
    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    
    std::string pps_path_;
    int fd_;
    std::mutex mutex_;
};

/**
 * Interface for receiving PPS notifications.
 */
class IPPSObserver {
public:
    virtual ~IPPSObserver() = default;
    virtual void onPPSData(const std::string& pps_path, const std::string& data) = 0;
    virtual void onPPSError(const std::string& pps_path, int error_code) = 0;
};

/**
 * A self-contained PPS subscriber that can be monitored via select().
 * Multiple subscribers can be managed by a multiplexer.
 */
class PPSSubscriber {
public:
    explicit PPSSubscriber(const std::string& pps_path)
        : pps_path_(pps_path), fd_(-1) {
        pps_decoder_initialize(&decoder_, nullptr);
    }
    
    ~PPSSubscriber() {
        close();
        pps_decoder_cleanup(&decoder_);
    }
    
    // Disable copy, enable move
    PPSSubscriber(const PPSSubscriber&) = delete;
    PPSSubscriber& operator=(const PPSSubscriber&) = delete;
    
    bool open() {
        if (fd_ >= 0) {
            return true;
        }
        
        // Open in delta mode to get only changes
        fd_ = ::open(pps_path_.c_str(), O_RDONLY);
        
        if (fd_ < 0) {
            return false;
        }
        
        // Set non-blocking for use with select()
        int flags = fcntl(fd_, F_GETFL);
        fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
        
        return true;
    }
    
    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    
    /**
     * Read available data. Call this when select() indicates data is ready.
     * Returns true if data was read successfully.
     */
    bool read(std::string& out_data) {
        if (fd_ < 0) {
            return false;
        }
        
        char buffer[4096];
        ssize_t bytes = ::read(fd_, buffer, sizeof(buffer) - 1);
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            out_data.assign(buffer, bytes);
            
            // Parse with PPS decoder for structured access
            pps_decoder_parse_pps_str(&decoder_, buffer);
            pps_decoder_push(&decoder_, nullptr);
            
            return true;
        } else if (bytes == 0) {
            // EOF - PPS object might have been deleted
            return false;
        } else {
            // Error or EAGAIN
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                return false;
            }
        }
        
        return false;
    }
    
    void addObserver(std::shared_ptr<IPPSObserver> observer) {
        std::lock_guard<std::mutex> lock(obs_mutex_);
        observers_.push_back(observer);
    }
    
    void notifyObservers(const std::string& data) {
        std::lock_guard<std::mutex> lock(obs_mutex_);
        for (auto& weak_obs : observers_) {
            if (auto obs = weak_obs.lock()) {
                obs->onPPSData(pps_path_, data);
            }
        }
        // Clean up expired observers
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                          [](const std::weak_ptr<IPPSObserver>& w) { return w.expired(); }),
            observers_.end());
    }
    
    void notifyError(int error_code) {
        std::lock_guard<std::mutex> lock(obs_mutex_);
        for (auto& weak_obs : observers_) {
            if (auto obs = weak_obs.lock()) {
                obs->onPPSError(pps_path_, error_code);
            }
        }
    }
    
    int getFd() const { return fd_; }
    const std::string& getPath() const { return pps_path_; }
    pps_decoder_t* getDecoder() { return &decoder_; }
    
private:
    std::string pps_path_;
    int fd_;
    pps_decoder_t decoder_;
    std::mutex obs_mutex_;
    std::vector<std::weak_ptr<IPPSObserver>> observers_;
};

/**
 * Multiplexes multiple PPS subscribers using select().
 * This runs in a dedicated thread and monitors all registered subscribers.
 */
class PPSMultiplexer {
public:
    PPSMultiplexer() : running_(false) {}
    
    ~PPSMultiplexer() {
        stop();
    }
    
    void start() {
        if (running_.exchange(true)) {
            return;
        }
        
        mux_thread_ = std::thread(&PPSMultiplexer::multiplexLoop, this);
    }
    
    void stop() {
        if (!running_.exchange(false)) {
            return;
        }
        
        // Create a self-pipe to wake up select()
        // In production, you'd set this up during initialization
        
        if (mux_thread_.joinable()) {
            mux_thread_.join();
        }
        
        std::lock_guard<std::mutex> lock(sub_mutex_);
        subscribers_.clear();
    }
    
    /**
     * Add a subscriber to be monitored. The subscriber must already be opened.
     */
    bool addSubscriber(std::shared_ptr<PPSSubscriber> subscriber) {
        if (!subscriber || subscriber->getFd() < 0) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(sub_mutex_);
        subscribers_.push_back(subscriber);
        return true;
    }
    
    /**
     * Remove a subscriber from monitoring.
     */
    void removeSubscriber(const std::string& pps_path) {
        std::lock_guard<std::mutex> lock(sub_mutex_);
        subscribers_.erase(
            std::remove_if(subscribers_.begin(), subscribers_.end(),
                          [&pps_path](const std::shared_ptr<PPSSubscriber>& sub) {
                              return sub->getPath() == pps_path;
                          }),
            subscribers_.end());
    }
    
private:
    void multiplexLoop() {
        fd_set read_fds;
        struct timeval timeout;
        
        while (running_) {
            FD_ZERO(&read_fds);
            int max_fd = -1;
            
            // Build the fd_set from all subscribers
            {
                std::lock_guard<std::mutex> lock(sub_mutex_);
                
                for (auto& sub : subscribers_) {
                    int fd = sub->getFd();
                    if (fd >= 0) {
                        FD_SET(fd, &read_fds);
                        max_fd = std::max(max_fd, fd);
                    }
                }
            }
            
            if (max_fd < 0) {
                // No file descriptors to monitor, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            // Use timeout to periodically check running_ flag
            timeout.tv_sec = 0;
            timeout.tv_usec = 500000; // 500ms
            
            int result = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
            
            if (result < 0) {
                if (errno == EINTR) {
                    continue;
                }
                // Handle error
                break;
            } else if (result == 0) {
                // Timeout - loop back to check running_ flag
                continue;
            }
            
            // Check which subscribers have data ready
            {
                std::lock_guard<std::mutex> lock(sub_mutex_);
                
                for (auto& sub : subscribers_) {
                    int fd = sub->getFd();
                    if (fd >= 0 && FD_ISSET(fd, &read_fds)) {
                        std::string data;
                        if (sub->read(data)) {
                            sub->notifyObservers(data);
                        } else {
                            sub->notifyError(errno);
                        }
                    }
                }
            }
        }
    }
    
    std::atomic<bool> running_;
    std::thread mux_thread_;
    std::mutex sub_mutex_;
    std::vector<std::shared_ptr<PPSSubscriber>> subscribers_;
};

/**
 * Alternative: Each view can manage its own subscriber in its own thread.
 * This gives maximum flexibility but requires careful resource management.
 */
class IndependentPPSMonitor {
public:
    explicit IndependentPPSMonitor(const std::string& pps_path)
        : subscriber_(std::make_shared<PPSSubscriber>(pps_path))
        , running_(false) {}
    
    ~IndependentPPSMonitor() {
        stop();
    }
    
    void start() {
        if (running_.exchange(true)) {
            return;
        }
        
        if (!subscriber_->open()) {
            running_ = false;
            return;
        }
        
        monitor_thread_ = std::thread(&IndependentPPSMonitor::monitorLoop, this);
    }
    
    void stop() {
        if (!running_.exchange(false)) {
            return;
        }
        
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        
        subscriber_->close();
    }
    
    void addObserver(std::shared_ptr<IPPSObserver> observer) {
        subscriber_->addObserver(observer);
    }
    
private:
    void monitorLoop() {
        fd_set read_fds;
        struct timeval timeout;
        int fd = subscriber_->getFd();
        
        while (running_) {
            FD_ZERO(&read_fds);
            FD_SET(fd, &read_fds);
            
            timeout.tv_sec = 0;
            timeout.tv_usec = 500000;
            
            int result = select(fd + 1, &read_fds, nullptr, nullptr, &timeout);
            
            if (result > 0 && FD_ISSET(fd, &read_fds)) {
                std::string data;
                if (subscriber_->read(data)) {
                    subscriber_->notifyObservers(data);
                } else {
                    subscriber_->notifyError(errno);
                }
            }
        }
    }
    
    std::shared_ptr<PPSSubscriber> subscriber_;
    std::atomic<bool> running_;
    std::thread monitor_thread_;
};

// ============================================================================
// Example Usage - Approach 2
// ============================================================================

class EngineModel {
public:
    EngineModel() : publisher_("/pps/vehicle/engine") {}
    
    void updateRPM(int rpm) {
        publisher_.publishAttribute("rpm", std::to_string(rpm), "n");
    }
    
    void updateTemperature(float temp) {
        publisher_.publishAttribute("temperature", std::to_string(temp), "n");
    }
    
private:
    PPSPublisher publisher_;
};

class GaugeView : public IPPSObserver, 
                  public std::enable_shared_from_this<GaugeView> {
public:
    GaugeView(PPSMultiplexer& mux) : mux_(mux) {
        // Create subscriber for engine data
        subscriber_ = std::make_shared<PPSSubscriber>("/pps/vehicle/engine");
        
        if (subscriber_->open()) {
            subscriber_->addObserver(shared_from_this());
            mux_.addSubscriber(subscriber_);
        }
    }
    
    void onPPSData(const std::string& pps_path, const std::string& data) override {
        // Parse PPS data and update gauge display
        pps_decoder_t* decoder = subscriber_->getDecoder();
        
        // Example: extract RPM value
        const char* rpm_str = pps_decoder_get_string(decoder, "rpm", nullptr);
        if (rpm_str) {
            printf("Gauge: RPM = %s\n", rpm_str);
        }
    }
    
    void onPPSError(const std::string& pps_path, int error_code) override {
        printf("Gauge: Error reading %s: %s\n", 
               pps_path.c_str(), strerror(error_code));
    }
    
private:
    PPSMultiplexer& mux_;
    std::shared_ptr<PPSSubscriber> subscriber_;
};

// Alternative: View with independent monitor
class StatusView : public IPPSObserver,
                   public std::enable_shared_from_this<StatusView> {
public:
    StatusView() : monitor_("/pps/vehicle/engine") {
        monitor_.addObserver(shared_from_this());
        monitor_.start();
    }
    
    ~StatusView() {
        monitor_.stop();
    }
    
    void onPPSData(const std::string& pps_path, const std::string& data) override {
        printf("Status: Received data from %s\n", pps_path.c_str());
    }
    
    void onPPSError(const std::string& pps_path, int error_code) override {
        printf("Status: Error on %s\n", pps_path.c_str());
    }
    
private:
    IndependentPPSMonitor monitor_;
};

int main() {
    // Example 1: Using centralized multiplexer
    {
        PPSMultiplexer multiplexer;
        multiplexer.start();
        
        EngineModel engine;
        auto gauge = std::make_shared<GaugeView>(multiplexer);
        
        for (int i = 0; i < 10; ++i) {
            engine.updateRPM(1000 + i * 100);
            engine.updateTemperature(80.0f + i * 2.0f);
            sleep(1);
        }
        
        multiplexer.stop();
    }
    
    // Example 2: Using independent monitors
    {
        EngineModel engine;
        auto status = std::make_shared<StatusView>();
        
        for (int i = 0; i < 10; ++i) {
            engine.updateRPM(1000 + i * 100);
            sleep(1);
        }
    }
    
    return 0;
}