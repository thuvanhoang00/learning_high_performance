#include <fcntl.h>
#include <sys/iofunc.h>
#include <sys/neutrino.h>
#include <sys/select.h>
#include <sys/pps.h>
#include <errno.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ============================================================================
// APPROACH 1: Single PPSManager with Threaded Reader/Writer
// ============================================================================

/**
 * Represents a message to be published to PPS.
 * Contains the PPS object path and the data to write.
 */
struct PPSMessage {
    std::string pps_path;
    std::string data;
    
    PPSMessage(const std::string& path, const std::string& d)
        : pps_path(path), data(d) {}
};

/**
 * Interface for views that subscribe to PPS updates.
 * Views implement this to receive notifications when their subscribed
 * PPS objects change.
 */
class IPPSSubscriber {
public:
    virtual ~IPPSSubscriber() = default;
    virtual void onPPSUpdate(const std::string& pps_path, const std::string& data) = 0;
    virtual void onPPSError(const std::string& pps_path, const std::string& error) = 0;
};

/**
 * Manages writing to PPS objects in a dedicated thread.
 * Models queue write requests, and the writer thread processes them asynchronously.
 */
class PPSWriter {
public:
    PPSWriter() : running_(false) {}
    
    ~PPSWriter() {
        stop();
    }
    
    void start() {
        if (running_.exchange(true)) {
            return; // Already running
        }
        
        writer_thread_ = std::thread(&PPSWriter::writerLoop, this);
    }
    
    void stop() {
        if (!running_.exchange(false)) {
            return; // Not running
        }
        
        // Wake up the writer thread so it can exit
        cv_.notify_one();
        
        if (writer_thread_.joinable()) {
            writer_thread_.join();
        }
        
        // Close all open file descriptors
        std::lock_guard<std::mutex> lock(fd_mutex_);
        for (auto& pair : pps_fds_) {
            if (pair.second >= 0) {
                close(pair.second);
            }
        }
        pps_fds_.clear();
    }
    
    /**
     * Queue a write operation. This is called by model objects.
     * Thread-safe and non-blocking.
     */
    void publish(const std::string& pps_path, const std::string& data) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            write_queue_.emplace(pps_path, data);
        }
        cv_.notify_one();
    }
    
private:
    void writerLoop() {
        while (running_) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // Wait for messages or shutdown signal
            cv_.wait(lock, [this] { 
                return !write_queue_.empty() || !running_; 
            });
            
            if (!running_) {
                break;
            }
            
            // Process all queued messages
            while (!write_queue_.empty()) {
                PPSMessage msg = std::move(write_queue_.front());
                write_queue_.pop();
                
                // Release lock while doing I/O
                lock.unlock();
                
                writeToPPS(msg.pps_path, msg.data);
                
                lock.lock();
            }
        }
    }
    
    void writeToPPS(const std::string& pps_path, const std::string& data) {
        int fd = getOrOpenPPS(pps_path, O_WRONLY);
        
        if (fd < 0) {
            // Log error - in production, notify error handler
            return;
        }
        
        // Write to PPS using the PPS format: attribute::value
        // For structured data, you might want to format as JSON or use PPS encoding
        ssize_t bytes_written = write(fd, data.c_str(), data.length());
        
        if (bytes_written < 0) {
            // Handle write error - might need to reopen the PPS object
            std::lock_guard<std::mutex> lock(fd_mutex_);
            close(fd);
            pps_fds_.erase(pps_path);
        }
    }
    
    int getOrOpenPPS(const std::string& pps_path, int flags) {
        std::lock_guard<std::mutex> lock(fd_mutex_);
        
        auto it = pps_fds_.find(pps_path);
        if (it != pps_fds_.end() && it->second >= 0) {
            return it->second;
        }
        
        // Open PPS object - use O_CREAT if you want to create it
        int fd = open(pps_path.c_str(), flags | O_CREAT, 0666);
        
        if (fd >= 0) {
            pps_fds_[pps_path] = fd;
        }
        
        return fd;
    }
    
    std::atomic<bool> running_;
    std::thread writer_thread_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::queue<PPSMessage> write_queue_;
    
    std::mutex fd_mutex_;
    std::unordered_map<std::string, int> pps_fds_;
};

/**
 * Manages reading from multiple PPS objects in a dedicated thread.
 * Uses ionotify() to efficiently monitor multiple file descriptors.
 */
class PPSReader {
public:
    PPSReader() : running_(false), chid_(-1), coid_(-1) {}
    
    ~PPSReader() {
        stop();
    }
    
    void start() {
        if (running_.exchange(true)) {
            return;
        }
        
        // Create channel for receiving notifications
        chid_ = ChannelCreate(0);
        if (chid_ < 0) {
            running_ = false;
            return;
        }
        
        coid_ = ConnectAttach(0, 0, chid_, _NTO_SIDE_CHANNEL, 0);
        if (coid_ < 0) {
            ChannelDestroy(chid_);
            chid_ = -1;
            running_ = false;
            return;
        }
        
        reader_thread_ = std::thread(&PPSReader::readerLoop, this);
    }
    
    void stop() {
        if (!running_.exchange(false)) {
            return;
        }
        
        // Send a pulse to wake up MsgReceive
        if (coid_ >= 0) {
            MsgSendPulse(coid_, -1, 0, 0);
        }
        
        if (reader_thread_.joinable()) {
            reader_thread_.join();
        }
        
        // Cleanup subscriptions
        std::lock_guard<std::mutex> lock(sub_mutex_);
        for (auto& pair : subscriptions_) {
            if (pair.second.fd >= 0) {
                close(pair.second.fd);
            }
        }
        subscriptions_.clear();
        
        if (coid_ >= 0) {
            ConnectDetach(coid_);
            coid_ = -1;
        }
        
        if (chid_ >= 0) {
            ChannelDestroy(chid_);
            chid_ = -1;
        }
    }
    
    /**
     * Subscribe to a PPS object. The subscriber will be notified of updates.
     */
    bool subscribe(const std::string& pps_path, std::shared_ptr<IPPSSubscriber> subscriber) {
        std::lock_guard<std::mutex> lock(sub_mutex_);
        
        // Check if already subscribed
        auto it = subscriptions_.find(pps_path);
        if (it != subscriptions_.end()) {
            it->second.subscribers.push_back(subscriber);
            return true;
        }
        
        // Open PPS object for reading with delta mode
        int fd = open(pps_path.c_str(), O_RDONLY);
        if (fd < 0) {
            return false;
        }
        
        // Set PPS to delta mode so we only get changes
        pps_decoder_t decoder;
        pps_decoder_initialize(&decoder, NULL);
        
        // Setup ionotify to monitor this file descriptor
        struct sigevent event;
        SIGEV_PULSE_INIT(&event, coid_, SIGEV_PULSE_PRIO_INHERIT, 
                         fd, 0);  // Use fd as pulse value to identify source
        
        if (ionotify(fd, _NOTIFY_ACTION_POLLARM, _NOTIFY_COND_INPUT, &event) < 0) {
            pps_decoder_cleanup(&decoder);
            close(fd);
            return false;
        }
        
        Subscription sub;
        sub.fd = fd;
        sub.decoder = decoder;
        sub.subscribers.push_back(subscriber);
        
        subscriptions_[pps_path] = sub;
        
        return true;
    }
    
private:
    struct Subscription {
        int fd;
        pps_decoder_t decoder;
        std::vector<std::shared_ptr<IPPSSubscriber>> subscribers;
    };
    
    void readerLoop() {
        struct _pulse pulse;
        
        while (running_) {
            // Wait for notification pulses
            int rcvid = MsgReceive(chid_, &pulse, sizeof(pulse), NULL);
            
            if (rcvid == 0) {
                // It's a pulse
                if (!running_) {
                    break;
                }
                
                int fd = pulse.value.sival_int;
                
                // Find which subscription this pulse is for
                std::string pps_path;
                {
                    std::lock_guard<std::mutex> lock(sub_mutex_);
                    for (const auto& pair : subscriptions_) {
                        if (pair.second.fd == fd) {
                            pps_path = pair.first;
                            break;
                        }
                    }
                }
                
                if (!pps_path.empty()) {
                    readAndNotify(pps_path);
                }
            } else if (rcvid > 0) {
                // Reply to unblock the sender
                MsgReply(rcvid, EOK, NULL, 0);
            }
        }
    }
    
    void readAndNotify(const std::string& pps_path) {
        std::lock_guard<std::mutex> lock(sub_mutex_);
        
        auto it = subscriptions_.find(pps_path);
        if (it == subscriptions_.end()) {
            return;
        }
        
        Subscription& sub = it->second;
        char buffer[4096];
        
        ssize_t bytes_read = read(sub.fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            
            // Parse PPS data
            pps_decoder_parse_pps_str(&sub.decoder, buffer);
            pps_decoder_push(&sub.decoder, NULL);
            
            // For simplicity, just pass raw data to subscribers
            // In production, you'd parse the PPS attributes
            std::string data(buffer, bytes_read);
            
            // Notify all subscribers
            for (auto& subscriber : sub.subscribers) {
                if (auto sub_ptr = subscriber.lock()) {
                    sub_ptr->onPPSUpdate(pps_path, data);
                }
            }
            
            // Re-arm the notification
            struct sigevent event;
            SIGEV_PULSE_INIT(&event, coid_, SIGEV_PULSE_PRIO_INHERIT, 
                           sub.fd, 0);
            ionotify(sub.fd, _NOTIFY_ACTION_POLLARM, _NOTIFY_COND_INPUT, &event);
            
        } else if (bytes_read < 0) {
            // Error handling
            for (auto& subscriber : sub.subscribers) {
                if (auto sub_ptr = subscriber.lock()) {
                    sub_ptr->onPPSError(pps_path, strerror(errno));
                }
            }
        }
    }
    
    std::atomic<bool> running_;
    std::thread reader_thread_;
    int chid_;  // Channel ID
    int coid_;  // Connection ID
    
    std::mutex sub_mutex_;
    std::unordered_map<std::string, Subscription> subscriptions_;
};

/**
 * Central manager that coordinates PPS reading and writing.
 * This is the single point of access for all models and views.
 */
class PPSManager {
public:
    PPSManager() = default;
    
    ~PPSManager() {
        stop();
    }
    
    void start() {
        writer_.start();
        reader_.start();
    }
    
    void stop() {
        writer_.stop();
        reader_.stop();
    }
    
    // Interface for models (publishers)
    void publish(const std::string& pps_path, const std::string& data) {
        writer_.publish(pps_path, data);
    }
    
    // Interface for views (subscribers)
    bool subscribe(const std::string& pps_path, std::shared_ptr<IPPSSubscriber> subscriber) {
        return reader_.subscribe(pps_path, subscriber);
    }
    
private:
    PPSWriter writer_;
    PPSReader reader_;
};

// ============================================================================
// Example Usage
// ============================================================================

class SensorModel {
public:
    SensorModel(PPSManager& manager) : manager_(manager) {}
    
    void updateTemperature(float temp) {
        // Format data for PPS - typically you'd use proper PPS encoding
        std::string data = "temperature:n:" + std::to_string(temp) + "\n";
        manager_.publish("/pps/sensors/temperature", data);
    }
    
private:
    PPSManager& manager_;
};

class DashboardView : public IPPSSubscriber {
public:
    DashboardView(PPSManager& manager) {
        // Subscribe to temperature updates
        manager.subscribe("/pps/sensors/temperature", 
                         std::shared_ptr<IPPSSubscriber>(this, [](IPPSSubscriber*){}));
    }
    
    void onPPSUpdate(const std::string& pps_path, const std::string& data) override {
        // Parse and update UI
        // In production, parse PPS attributes properly
        printf("Dashboard received update from %s: %s\n", 
               pps_path.c_str(), data.c_str());
    }
    
    void onPPSError(const std::string& pps_path, const std::string& error) override {
        printf("Dashboard error on %s: %s\n", pps_path.c_str(), error.c_str());
    }
};

int main() {
    PPSManager manager;
    manager.start();
    
    SensorModel sensor(manager);
    DashboardView dashboard(manager);
    
    // Simulate sensor updates
    for (int i = 0; i < 10; ++i) {
        sensor.updateTemperature(20.0f + i * 0.5f);
        sleep(1);
    }
    
    manager.stop();
    return 0;
}