#include <fcntl.h>
#include <sys/neutrino.h>
#include <sys/pps.h>
#include <unistd.h>
#include <errno.h>

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
#include <algorithm>
#include <iostream>

// ============================================================================
// Generic Type-Safe Callback System for PPS
// ============================================================================

/**
 * Base interface for all PPS subscribers.
 * This is templated on the data type, ensuring compile-time type safety.
 * 
 * Each data type gets its own subscriber interface, which means you can't
 * accidentally register a SensorData subscriber for EngineData updates.
 */
template<typename DataType>
class IPPSSubscriber {
public:
    virtual ~IPPSSubscriber() = default;
    
    /**
     * Called when PPS data is available.
     * The data is passed by const reference for zero-copy efficiency.
     */
    virtual void onPPSUpdate(const std::string& pps_path, 
                            const DataType& data) = 0;
    
    /**
     * Called when an error occurs reading from PPS.
     */
    virtual void onPPSError(const std::string& pps_path, 
                           const std::string& error) = 0;
};

/**
 * Manages subscriptions for a specific data type.
 * This is the internal component that handles the subscriber list
 * and notification logic.
 */
template<typename DataType>
class SubscriptionManager {
public:
    using SubscriberPtr = std::shared_ptr<IPPSSubscriber<DataType>>;
    using WeakSubscriberPtr = std::weak_ptr<IPPSSubscriber<DataType>>;
    
    /**
     * Add a subscriber for a specific PPS path.
     * We store weak_ptr so subscribers can be destroyed without notification.
     */
    void subscribe(const std::string& pps_path, SubscriberPtr subscriber) {
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptions_[pps_path].push_back(subscriber);
    }
    
    /**
     * Remove a subscriber from a specific PPS path.
     */
    void unsubscribe(const std::string& pps_path, SubscriberPtr subscriber) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = subscriptions_.find(pps_path);
        if (it == subscriptions_.end()) {
            return;
        }
        
        auto& subs = it->second;
        subs.erase(
            std::remove_if(subs.begin(), subs.end(),
                          [&subscriber](const WeakSubscriberPtr& weak) {
                              auto ptr = weak.lock();
                              return ptr == subscriber;
                          }),
            subs.end()
        );
    }
    
    /**
     * Notify all subscribers for a PPS path with new data.
     * This is where the magic happens:
     * 1. We lock() each weak_ptr to get a shared_ptr
     * 2. If lock() succeeds, the subscriber exists and we call it
     * 3. If lock() fails, the subscriber was destroyed - we skip it
     * 4. We clean up expired weak_ptrs after notification
     */
    void notify(const std::string& pps_path, const DataType& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = subscriptions_.find(pps_path);
        if (it == subscriptions_.end()) {
            return;
        }
        
        auto& subscribers = it->second;
        
        // Notify all living subscribers
        for (auto& weak_sub : subscribers) {
            // This is the critical lifetime check!
            // lock() returns shared_ptr if object exists, nullptr if destroyed
            if (auto sub = weak_sub.lock()) {
                sub->onPPSUpdate(pps_path, data);
            }
        }
        
        // Clean up destroyed subscribers
        // This is why we use weak_ptr - automatic cleanup!
        subscribers.erase(
            std::remove_if(subscribers.begin(), subscribers.end(),
                          [](const WeakSubscriberPtr& weak) {
                              return weak.expired();
                          }),
            subscribers.end()
        );
    }
    
    /**
     * Notify subscribers of an error.
     */
    void notifyError(const std::string& pps_path, const std::string& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = subscriptions_.find(pps_path);
        if (it == subscriptions_.end()) {
            return;
        }
        
        for (auto& weak_sub : it->second) {
            if (auto sub = weak_sub.lock()) {
                sub->onPPSError(pps_path, error);
            }
        }
    }
    
    /**
     * Check if any subscribers exist for a path.
     */
    bool hasSubscribers(const std::string& pps_path) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscriptions_.find(pps_path);
        return it != subscriptions_.end() && !it->second.empty();
    }
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<WeakSubscriberPtr>> subscriptions_;
};

// ============================================================================
// PPS Reader with Generic Type Support
// ============================================================================

/**
 * Generic PPS reader that can handle any data type.
 * 
 * The key insight: Each data type has its own SubscriptionManager.
 * When you call subscribe<SensorData>(), it goes to the SensorData manager.
 * When you notify<SensorData>(), it notifies SensorData subscribers.
 * 
 * This provides complete type safety at compile time with zero runtime overhead.
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
        
        if (coid_ >= 0) {
            MsgSendPulse(coid_, -1, 0, 0);
        }
        
        if (reader_thread_.joinable()) {
            reader_thread_.join();
        }
        
        std::lock_guard<std::mutex> lock(fd_mutex_);
        for (auto& pair : pps_fds_) {
            close(pair.second);
        }
        pps_fds_.clear();
        
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
     * Subscribe to a PPS object with type-safe callbacks.
     * 
     * Template parameter DataType determines which data type this subscription handles.
     * The subscriber must implement IPPSSubscriber<DataType>.
     * 
     * Example:
     *   auto view = std::make_shared<SensorView>();
     *   reader.subscribe<SensorData>("/pps/sensors/temp", view);
     */
    template<typename DataType>
    bool subscribe(const std::string& pps_path,
                   std::shared_ptr<IPPSSubscriber<DataType>> subscriber) {
        // Get or create the subscription manager for this type
        auto& manager = getSubscriptionManager<DataType>();
        
        // Add subscriber to the manager
        manager.subscribe(pps_path, subscriber);
        
        // Open PPS file if not already open
        return openPPSObject(pps_path);
    }
    
    /**
     * Unsubscribe from a PPS object.
     */
    template<typename DataType>
    void unsubscribe(const std::string& pps_path,
                     std::shared_ptr<IPPSSubscriber<DataType>> subscriber) {
        auto& manager = getSubscriptionManager<DataType>();
        manager.unsubscribe(pps_path, subscriber);
    }
    
    /**
     * Manually notify subscribers with parsed data.
     * This is called after reading and parsing PPS data.
     */
    template<typename DataType>
    void notify(const std::string& pps_path, const DataType& data) {
        auto& manager = getSubscriptionManager<DataType>();
        manager.notify(pps_path, data);
    }
    
    /**
     * Register a parser function for a PPS path.
     * The parser converts raw PPS string data into a typed object.
     * 
     * Example:
     *   reader.registerParser<SensorData>("/pps/sensors/temp",
     *       [](const std::string& raw) -> SensorData {
     *           // Parse raw PPS data and return SensorData object
     *       });
     */
    template<typename DataType>
    void registerParser(const std::string& pps_path,
                       std::function<DataType(const std::string&)> parser) {
        std::lock_guard<std::mutex> lock(parser_mutex_);
        
        // Store parser with type erasure
        parsers_[pps_path] = [this, pps_path, parser](const std::string& raw) {
            try {
                DataType data = parser(raw);
                this->notify(pps_path, data);
            } catch (const std::exception& e) {
                auto& manager = this->getSubscriptionManager<DataType>();
                manager.notifyError(pps_path, std::string("Parse error: ") + e.what());
            }
        };
    }
    
private:
    /**
     * Get the subscription manager for a specific data type.
     * Each type gets its own manager, stored in a type-indexed map.
     * 
     * This is the type-safe magic: The template parameter ensures you
     * get the correct manager for your data type at compile time.
     */
    template<typename DataType>
    SubscriptionManager<DataType>& getSubscriptionManager() {
        static SubscriptionManager<DataType> manager;
        return manager;
    }
    
    bool openPPSObject(const std::string& pps_path) {
        std::lock_guard<std::mutex> lock(fd_mutex_);
        
        // Check if already open
        if (pps_fds_.find(pps_path) != pps_fds_.end()) {
            return true;
        }
        
        // Open PPS object
        int fd = open(pps_path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            return false;
        }
        
        // Setup ionotify
        struct sigevent event;
        SIGEV_PULSE_INIT(&event, coid_, SIGEV_PULSE_PRIO_INHERIT, fd, 0);
        
        if (ionotify(fd, _NOTIFY_ACTION_POLLARM, _NOTIFY_COND_INPUT, &event) < 0) {
            close(fd);
            return false;
        }
        
        pps_fds_[pps_path] = fd;
        fd_to_path_[fd] = pps_path;
        
        return true;
    }
    
    void readerLoop() {
        struct _pulse pulse;
        
        while (running_) {
            int rcvid = MsgReceive(chid_, &pulse, sizeof(pulse), NULL);
            
            if (rcvid == 0) {
                if (!running_) break;
                
                int fd = pulse.value.sival_int;
                handlePPSData(fd);
            } else if (rcvid > 0) {
                MsgReply(rcvid, EOK, NULL, 0);
            }
        }
    }
    
    void handlePPSData(int fd) {
        // Find PPS path for this fd
        std::string pps_path;
        {
            std::lock_guard<std::mutex> lock(fd_mutex_);
            auto it = fd_to_path_.find(fd);
            if (it != fd_to_path_.end()) {
                pps_path = it->second;
            }
        }
        
        if (pps_path.empty()) {
            return;
        }
        
        // Read data
        char buffer[4096];
        ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string raw_data(buffer, bytes);
            
            // Find and invoke parser
            std::lock_guard<std::mutex> lock(parser_mutex_);
            auto it = parsers_.find(pps_path);
            if (it != parsers_.end()) {
                it->second(raw_data);
            }
            
            // Re-arm notification
            struct sigevent event;
            SIGEV_PULSE_INIT(&event, coid_, SIGEV_PULSE_PRIO_INHERIT, fd, 0);
            ionotify(fd, _NOTIFY_ACTION_POLLARM, _NOTIFY_COND_INPUT, &event);
        }
    }
    
    std::atomic<bool> running_;
    std::thread reader_thread_;
    int chid_;
    int coid_;
    
    std::mutex fd_mutex_;
    std::unordered_map<std::string, int> pps_fds_;
    std::unordered_map<int, std::string> fd_to_path_;
    
    std::mutex parser_mutex_;
    std::unordered_map<std::string, std::function<void(const std::string&)>> parsers_;
};

// ============================================================================
// Example Usage with Different Data Types
// ============================================================================

// Define your data structures
struct SensorData {
    float temperature;
    float humidity;
    
    void print() const {
        std::cout << "Temp: " << temperature << "°C, Humidity: " << humidity << "%" << std::endl;
    }
};

struct EngineData {
    int rpm;
    float oil_pressure;
    
    void print() const {
        std::cout << "RPM: " << rpm << ", Oil: " << oil_pressure << " PSI" << std::endl;
    }
};

// Implement views as subscribers
class SensorView : public IPPSSubscriber<SensorData>,
                   public std::enable_shared_from_this<SensorView> {
public:
    SensorView(const std::string& name) : name_(name) {}
    
    void onPPSUpdate(const std::string& pps_path, 
                    const SensorData& data) override {
        std::cout << "[" << name_ << "] Sensor update from " << pps_path << ": ";
        data.print();
    }
    
    void onPPSError(const std::string& pps_path, 
                   const std::string& error) override {
        std::cout << "[" << name_ << "] Error on " << pps_path << ": " << error << std::endl;
    }
    
private:
    std::string name_;
};

class EngineView : public IPPSSubscriber<EngineData>,
                   public std::enable_shared_from_this<EngineView> {
public:
    void onPPSUpdate(const std::string& pps_path, 
                    const EngineData& data) override {
        std::cout << "[Engine View] Update: ";
        data.print();
    }
    
    void onPPSError(const std::string& pps_path, 
                   const std::string& error) override {
        std::cout << "[Engine View] Error: " << error << std::endl;
    }
};

int main() {
    PPSReader reader;
    reader.start();
    
    // Register parsers for different data types
    reader.registerParser<SensorData>("/pps/sensors/temp",
        [](const std::string& raw) -> SensorData {
            SensorData data;
            // Parse PPS format: temperature:n:22.5\nhumidity:n:65.0\n
            sscanf(raw.c_str(), "temperature:n:%f\nhumidity:n:%f", 
                   &data.temperature, &data.humidity);
            return data;
        });
    
    reader.registerParser<EngineData>("/pps/engine/stats",
        [](const std::string& raw) -> EngineData {
            EngineData data;
            sscanf(raw.c_str(), "rpm:n:%d\noil_pressure:n:%f", 
                   &data.rpm, &data.oil_pressure);
            return data;
        });
    
    // Create views and subscribe
    // Using shared_ptr for automatic lifetime management!
    auto sensor_view1 = std::make_shared<SensorView>("View1");
    auto sensor_view2 = std::make_shared<SensorView>("View2");
    auto engine_view = std::make_shared<EngineView>();
    
    reader.subscribe<SensorData>("/pps/sensors/temp", sensor_view1);
    reader.subscribe<SensorData>("/pps/sensors/temp", sensor_view2);
    reader.subscribe<EngineData>("/pps/engine/stats", engine_view);
    
    std::cout << "Subscribers registered. In real system, PPS updates would trigger callbacks.\n";
    std::cout << "\nDestroying view2...\n";
    sensor_view2.reset();  // Destroy view2
    std::cout << "View2 destroyed. Next notification will only reach view1.\n";
    
    // Simulate notifications (in real system, these come from PPS reads)
    SensorData sensor_data{22.5f, 65.0f};
    reader.notify("/pps/sensors/temp", sensor_data);
    
    EngineData engine_data{3000, 45.5f};
    reader.notify("/pps/engine/stats", engine_data);
    
    reader.stop();
    return 0;
}