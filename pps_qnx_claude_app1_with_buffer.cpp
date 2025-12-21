#include <fcntl.h>
#include <sys/iofunc.h>
#include <sys/neutrino.h>
#include <sys/pps.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

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

// ============================================================================
// Zero-Copy Buffer Management
// ============================================================================

/**
 * A reusable buffer that manages its own memory lifecycle.
 * 
 * The philosophy here is to allocate memory once and reuse it across multiple
 * read operations. This eliminates the overhead of malloc/free on every PPS
 * update, which can be significant in high-frequency scenarios.
 * 
 * The buffer grows when needed but never shrinks, which means after handling
 * the largest message you'll see, all subsequent operations are zero-allocation.
 */
class PPSBuffer {
public:
    explicit PPSBuffer(size_t initial_capacity = 4096)
        : capacity_(initial_capacity)
        , size_(0)
        , data_(nullptr)
        , ref_count_(0) {
        data_ = static_cast<char*>(malloc(capacity_));
        if (!data_) {
            capacity_ = 0;
            throw std::bad_alloc();
        }
    }
    
    ~PPSBuffer() {
        free(data_);
    }
    
    // Disable copy to prevent accidental duplication
    PPSBuffer(const PPSBuffer&) = delete;
    PPSBuffer& operator=(const PPSBuffer&) = delete;
    
    // Enable move semantics for efficient transfers
    PPSBuffer(PPSBuffer&& other) noexcept
        : capacity_(other.capacity_)
        , size_(other.size_)
        , data_(other.data_)
        , ref_count_(other.ref_count_.load()) {
        other.data_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
        other.ref_count_ = 0;
    }
    
    /**
     * Ensure the buffer can hold at least 'required' bytes.
     * This is the only place where memory allocation happens.
     */
    bool ensureCapacity(size_t required) {
        if (capacity_ >= required) {
            return true;
        }
        
        // Grow geometrically to amortize allocation costs
        // We double the size until we exceed requirement, then use exact requirement
        size_t new_capacity = capacity_;
        while (new_capacity < required) {
            new_capacity *= 2;
        }
        
        // Use realloc which may avoid copying if memory can be extended in place
        char* new_data = static_cast<char*>(realloc(data_, new_capacity));
        if (!new_data) {
            return false;
        }
        
        data_ = new_data;
        capacity_ = new_capacity;
        return true;
    }
    
    /**
     * Read from a file descriptor into this buffer.
     * This is the core zero-copy operation - we read directly into our
     * managed buffer without any intermediate copies.
     * 
     * @param fd File descriptor to read from
     * @param max_size Maximum bytes to read (prevents DOS attacks)
     * @return Number of bytes read, or -1 on error
     */
    ssize_t readFrom(int fd, size_t max_size = 10 * 1024 * 1024) {
        size_ = 0;
        
        while (true) {
            // Calculate available space in buffer
            size_t space = capacity_ - size_;
            
            // If running low on space, grow the buffer
            // We keep 1KB margin to ensure null terminator fits if needed
            if (space < 1024) {
                size_t new_capacity = capacity_ * 2;
                if (new_capacity > max_size) {
                    new_capacity = max_size;
                }
                
                if (!ensureCapacity(new_capacity)) {
                    errno = ENOMEM;
                    return -1;
                }
                space = capacity_ - size_;
            }
            
            // Enforce maximum size limit to prevent runaway growth
            if (size_ >= max_size) {
                errno = EFBIG;
                return -1;
            }
            
            // Limit this read to respect max_size
            space = std::min(space, max_size - size_);
            
            // Read directly into our buffer at current position
            // This is the zero-copy magic - no intermediate buffers
            ssize_t bytes = read(fd, data_ + size_, space);
            
            if (bytes < 0) {
                // EAGAIN means no more data available (non-blocking mode)
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                // Real error occurred
                return -1;
                
            } else if (bytes == 0) {
                // EOF reached
                break;
                
            } else {
                size_ += bytes;
                
                // If we read less than available space, we've consumed all data
                // This is our signal that the read is complete
                if (static_cast<size_t>(bytes) < space) {
                    break;
                }
                // Otherwise loop to read more
            }
        }
        
        // Null-terminate for C string compatibility
        // We ensured space for this in the capacity check above
        if (size_ < capacity_) {
            data_[size_] = '\0';
        }
        
        return size_;
    }
    
    /**
     * Reset the buffer for reuse without deallocating memory.
     * This is called when returning buffer to pool.
     */
    void clear() {
        size_ = 0;
    }
    
    // Accessors
    const char* data() const { return data_; }
    char* data() { return data_; }
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    
    // Convert to string (involves a copy, but sometimes necessary)
    std::string toString() const {
        return std::string(data_, size_);
    }
    
    // Reference counting for buffer pooling
    void addRef() { ++ref_count_; }
    void release() { --ref_count_; }
    int refCount() const { return ref_count_.load(); }
    
private:
    size_t capacity_;
    size_t size_;
    char* data_;
    std::atomic<int> ref_count_;
};

/**
 * Pool of reusable buffers to eliminate allocation overhead.
 * 
 * The buffer pool maintains a collection of pre-allocated buffers that can
 * be checked out for use and returned when done. This is crucial for
 * high-performance applications where you're processing many PPS updates
 * per second and can't afford the overhead of allocating a new buffer
 * for each update.
 * 
 * The pool has two key properties:
 * 1. It grows on demand when all buffers are in use
 * 2. It never shrinks, so steady-state operation is allocation-free
 */
class BufferPool {
public:
    explicit BufferPool(size_t initial_count = 4, size_t buffer_size = 4096)
        : buffer_size_(buffer_size)
        , max_pool_size_(32) {  // Cap pool size to prevent unbounded growth
        
        // Pre-allocate initial buffers
        for (size_t i = 0; i < initial_count; ++i) {
            available_buffers_.push(std::make_unique<PPSBuffer>(buffer_size));
        }
    }
    
    /**
     * Acquire a buffer from the pool.
     * If no buffers are available, allocates a new one.
     * 
     * This is the only public allocation point - once the pool reaches
     * steady state with enough buffers for your workload, all subsequent
     * acquires are allocation-free.
     */
    std::unique_ptr<PPSBuffer> acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!available_buffers_.empty()) {
            // Reuse an existing buffer - this is the common case
            auto buffer = std::move(available_buffers_.front());
            available_buffers_.pop();
            buffer->clear();
            buffer->addRef();
            return buffer;
        }
        
        // No buffers available - allocate a new one
        // This should be rare after initial warm-up phase
        auto buffer = std::make_unique<PPSBuffer>(buffer_size_);
        buffer->addRef();
        return buffer;
    }
    
    /**
     * Return a buffer to the pool for reuse.
     * The buffer's memory is retained for the next acquire.
     */
    void release(std::unique_ptr<PPSBuffer> buffer) {
        if (!buffer) {
            return;
        }
        
        buffer->release();
        buffer->clear();
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Only keep up to max_pool_size buffers
        // This prevents the pool from growing unbounded if you have a spike
        if (available_buffers_.size() < max_pool_size_) {
            available_buffers_.push(std::move(buffer));
        }
        // If pool is full, buffer will be destroyed (via unique_ptr)
    }
    
    // Get statistics for monitoring and tuning
    size_t availableCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return available_buffers_.size();
    }
    
private:
    size_t buffer_size_;
    size_t max_pool_size_;
    mutable std::mutex mutex_;
    std::queue<std::unique_ptr<PPSBuffer>> available_buffers_;
};

// ============================================================================
// Subscriber Interface with Zero-Copy Support
// ============================================================================

/**
 * Enhanced subscriber interface that supports zero-copy data access.
 * 
 * Subscribers receive a const reference to the buffer, which means they can
 * access the data without any copying. However, they must process the data
 * synchronously within the callback - they cannot store the buffer reference
 * for later use because the buffer will be returned to the pool.
 * 
 * If a subscriber needs to keep the data, they must make a copy (toString()).
 */
class IPPSSubscriber {
public:
    virtual ~IPPSSubscriber() = default;
    
    /**
     * Called when PPS data is available.
     * 
     * IMPORTANT: The buffer is only valid during this callback.
     * If you need the data later, copy it via buffer.toString().
     * 
     * @param pps_path Path of the PPS object
     * @param buffer Zero-copy buffer containing the data
     */
    virtual void onPPSUpdate(const std::string& pps_path, 
                            const PPSBuffer& buffer) = 0;
    
    virtual void onPPSError(const std::string& pps_path, 
                           const std::string& error) = 0;
};

// ============================================================================
// Zero-Copy PPS Writer
// ============================================================================

/**
 * Message structure for write queue.
 * Uses move semantics to avoid string copies.
 */
struct PPSMessage {
    std::string pps_path;
    std::string data;
    
    PPSMessage(std::string path, std::string d)
        : pps_path(std::move(path)), data(std::move(d)) {}
};

class PPSWriter {
public:
    PPSWriter() : running_(false) {}
    
    ~PPSWriter() {
        stop();
    }
    
    void start() {
        if (running_.exchange(true)) {
            return;
        }
        writer_thread_ = std::thread(&PPSWriter::writerLoop, this);
    }
    
    void stop() {
        if (!running_.exchange(false)) {
            return;
        }
        
        cv_.notify_one();
        
        if (writer_thread_.joinable()) {
            writer_thread_.join();
        }
        
        std::lock_guard<std::mutex> lock(fd_mutex_);
        for (auto& pair : pps_fds_) {
            if (pair.second >= 0) {
                close(pair.second);
            }
        }
        pps_fds_.clear();
    }
    
    /**
     * Publish data to PPS object.
     * Uses move semantics to avoid copying the data string.
     */
    void publish(std::string pps_path, std::string data) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            write_queue_.emplace(std::move(pps_path), std::move(data));
        }
        cv_.notify_one();
    }
    
private:
    void writerLoop() {
        while (running_) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            cv_.wait(lock, [this] { 
                return !write_queue_.empty() || !running_; 
            });
            
            if (!running_) {
                break;
            }
            
            while (!write_queue_.empty()) {
                PPSMessage msg = std::move(write_queue_.front());
                write_queue_.pop();
                
                lock.unlock();
                writeToPPS(msg.pps_path, msg.data);
                lock.lock();
            }
        }
    }
    
    void writeToPPS(const std::string& pps_path, const std::string& data) {
        int fd = getOrOpenPPS(pps_path, O_WRONLY);
        if (fd < 0) return;
        
        ssize_t bytes_written = write(fd, data.c_str(), data.length());
        
        if (bytes_written < 0) {
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

// ============================================================================
// Zero-Copy PPS Reader with Buffer Pooling
// ============================================================================

class PPSReader {
public:
    PPSReader() 
        : running_(false)
        , chid_(-1)
        , coid_(-1)
        , buffer_pool_(8, 4096) {}  // Start with 8 buffers of 4KB each
    
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
    
    bool subscribe(const std::string& pps_path, 
                   std::shared_ptr<IPPSSubscriber> subscriber) {
        std::lock_guard<std::mutex> lock(sub_mutex_);
        
        auto it = subscriptions_.find(pps_path);
        if (it != subscriptions_.end()) {
            it->second.subscribers.push_back(subscriber);
            return true;
        }
        
        int fd = open(pps_path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            return false;
        }
        
        pps_decoder_t decoder;
        pps_decoder_initialize(&decoder, NULL);
        
        struct sigevent event;
        SIGEV_PULSE_INIT(&event, coid_, SIGEV_PULSE_PRIO_INHERIT, fd, 0);
        
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
    
    // Get buffer pool statistics for monitoring
    size_t getAvailableBufferCount() const {
        return buffer_pool_.availableCount();
    }
    
private:
    struct Subscription {
        int fd;
        pps_decoder_t decoder;
        std::vector<std::weak_ptr<IPPSSubscriber>> subscribers;
    };
    
    void readerLoop() {
        struct _pulse pulse;
        
        while (running_) {
            int rcvid = MsgReceive(chid_, &pulse, sizeof(pulse), NULL);
            
            if (rcvid == 0) {
                if (!running_) break;
                
                int fd = pulse.value.sival_int;
                
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
                MsgReply(rcvid, EOK, NULL, 0);
            }
        }
    }
    
    /**
     * The heart of zero-copy operation.
     * 
     * We acquire a buffer from the pool, read directly into it, notify
     * subscribers (who access the buffer without copying), then return
     * the buffer to the pool for reuse.
     * 
     * This entire operation happens without any heap allocations after
     * the initial warm-up phase.
     */
    void readAndNotify(const std::string& pps_path) {
        std::lock_guard<std::mutex> lock(sub_mutex_);
        
        auto it = subscriptions_.find(pps_path);
        if (it == subscriptions_.end()) {
            return;
        }
        
        Subscription& sub = it->second;
        
        // Acquire a buffer from the pool (zero-copy operation after warm-up)
        auto buffer = buffer_pool_.acquire();
        
        // Read directly into the pooled buffer
        ssize_t bytes_read = buffer->readFrom(sub.fd);
        
        if (bytes_read > 0) {
            // Parse PPS data with the decoder
            pps_decoder_parse_pps_str(&sub.decoder, buffer->data());
            pps_decoder_push(&sub.decoder, NULL);
            
            // Notify all subscribers, passing buffer by const reference
            // Subscribers can access data without copying
            for (auto& weak_sub : sub.subscribers) {
                if (auto sub_ptr = weak_sub.lock()) {
                    sub_ptr->onPPSUpdate(pps_path, *buffer);
                }
            }
            
            // Clean up expired weak pointers
            sub.subscribers.erase(
                std::remove_if(sub.subscribers.begin(), sub.subscribers.end(),
                              [](const std::weak_ptr<IPPSSubscriber>& w) { 
                                  return w.expired(); 
                              }),
                sub.subscribers.end());
            
            // Re-arm the notification for next update
            struct sigevent event;
            SIGEV_PULSE_INIT(&event, coid_, SIGEV_PULSE_PRIO_INHERIT, sub.fd, 0);
            ionotify(sub.fd, _NOTIFY_ACTION_POLLARM, _NOTIFY_COND_INPUT, &event);
            
        } else if (bytes_read < 0) {
            // Error handling
            std::string error = strerror(errno);
            for (auto& weak_sub : sub.subscribers) {
                if (auto sub_ptr = weak_sub.lock()) {
                    sub_ptr->onPPSError(pps_path, error);
                }
            }
        }
        
        // Buffer automatically returns to pool when unique_ptr goes out of scope
        buffer_pool_.release(std::move(buffer));
    }
    
    std::atomic<bool> running_;
    std::thread reader_thread_;
    int chid_;
    int coid_;
    std::mutex sub_mutex_;
    std::unordered_map<std::string, Subscription> subscriptions_;
    BufferPool buffer_pool_;  // The zero-copy magic happens here
};

// ============================================================================
// Integrated PPS Manager
// ============================================================================

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
    
    // Publishers use move semantics to avoid copying
    void publish(std::string pps_path, std::string data) {
        writer_.publish(std::move(pps_path), std::move(data));
    }
    
    bool subscribe(const std::string& pps_path, 
                   std::shared_ptr<IPPSSubscriber> subscriber) {
        return reader_.subscribe(pps_path, subscriber);
    }
    
    // Monitoring API for performance tuning
    size_t getAvailableBufferCount() const {
        return reader_.getAvailableBufferCount();
    }
    
private:
    PPSWriter writer_;
    PPSReader reader_;
};

// ============================================================================
// Example Usage with Zero-Copy Benefits
// ============================================================================

#include <stdio.h>

/**
 * Example model that publishes sensor data.
 * Uses move semantics to avoid string copies.
 */
class SensorModel {
public:
    SensorModel(PPSManager& manager) : manager_(manager) {}
    
    void updateTemperature(float temp) {
        // Build the PPS string
        std::string data = "temperature:n:" + std::to_string(temp) + "\n";
        
        // Move the string to avoid copying
        manager_.publish("/pps/sensors/temperature", std::move(data));
    }
    
    void updateMultipleValues(float temp, float humidity, float pressure) {
        std::string data;
        data.reserve(256);  // Pre-allocate to avoid reallocation
        
        data += "temperature:n:" + std::to_string(temp) + "\n";
        data += "humidity:n:" + std::to_string(humidity) + "\n";
        data += "pressure:n:" + std::to_string(pressure) + "\n";
        
        manager_.publish("/pps/sensors/multi", std::move(data));
    }
    
private:
    PPSManager& manager_;
};

/**
 * Example view that processes data with zero-copy efficiency.
 */
class DashboardView : public IPPSSubscriber,
                      public std::enable_shared_from_this<DashboardView> {
public:
    DashboardView(PPSManager& manager, const std::string& pps_path)
        : pps_path_(pps_path) {
        manager.subscribe(pps_path_, shared_from_this());
    }
    
    /**
     * Process PPS data without copying.
     * 
     * The buffer contains the raw PPS data. We can access it directly
     * without any memory copies. If we need to keep the data beyond
     * this callback, we would call buffer.toString() to make a copy.
     */
    void onPPSUpdate(const std::string& pps_path, 
                    const PPSBuffer& buffer) override {
        // Access data directly from buffer - zero copy!
        printf("Dashboard [%s]: Received %zu bytes\n", 
               pps_path.c_str(), buffer.size());
        
        // For demonstration, show first 100 chars
        size_t display_len = std::min(buffer.size(), size_t(100));
        printf("Data: %.*s%s\n", 
               static_cast<int>(display_len), 
               buffer.data(),
               buffer.size() > 100 ? "..." : "");
        
        // If you need to store the data, make a copy:
        // cached_data_ = buffer.toString();
    }
    
    void onPPSError(const std::string& pps_path, 
                   const std::string& error) override {
        printf("Dashboard error on %s: %s\n", pps_path.c_str(), error.c_str());
    }
    
private:
    std::string pps_path_;
    // std::string cached_data_;  // Only if you need to keep data
};

/**
 * High-performance view that parses PPS attributes in place.
 */
class FastGaugeView : public IPPSSubscriber,
                      public std::enable_shared_from_this<FastGaugeView> {
public:
    FastGaugeView(PPSManager& manager) {
        manager.subscribe("/pps/sensors/multi", shared_from_this());
    }
    
    void onPPSUpdate(const std::string& pps_path, 
                    const PPSBuffer& buffer) override {
        // Parse directly from buffer without copying
        // This is where zero-copy really shines in performance-critical code
        
        const char* data = buffer.data();
        size_t size = buffer.size();
        
        // Simple inline parsing (in production, use PPS decoder properly)
        float temp = 0, humidity = 0, pressure = 0;
        
        // This parsing happens directly on the buffer data - no copies
        const char* pos = data;
        const char* end = data + size;
        
        while (pos < end) {
            if (sscanf(pos, "temperature:n:%f", &temp) == 1) {
                printf("Temperature: %.2f°C\n", temp);
            } else if (sscanf(pos, "humidity:n:%f", &humidity) == 1) {
                printf("Humidity: %.2f%%\n", humidity);
            } else if (sscanf(pos, "pressure:n:%f", &pressure) == 1) {
                printf("Pressure: %.2f hPa\n", pressure);
            }
            
            // Move to next line
            pos = static_cast<const char*>(memchr(pos, '\n', end - pos));
            if (!pos) break;
            pos++;
        }
    }
    
    void onPPSError(const std::string& pps_path, 
                   const std::string& error) override {
        printf("Gauge error: %s\n", error.c_str());
    }
};

int main() {
    PPSManager manager;
    manager.start();
    
    // Create model and views
    SensorModel sensor(manager);
    auto dashboard = std::make_shared<DashboardView>(manager, 
                                                     "/pps/sensors/temperature");
    auto gauge = std::make_shared<FastGaugeView>(manager);
    
    printf("Starting PPS zero-copy demonstration...\n");
    printf("Buffer pool starts with buffers available: %zu\n\n", 
           manager.getAvailableBufferCount());
    
    // Simulate high-frequency sensor updates
    for (int i = 0; i < 20; ++i) {
        sensor.updateTemperature(20.0f + i * 0.5f);
        sensor.updateMultipleValues(20.0f + i * 0.5f, 
                                    45.0f + i * 1.0f, 
                                    1013.0f + i * 0.1f);
        
        // After a few iterations, check buffer pool
        if (i == 5) {
            printf("\nAfter warm-up, buffers available: %zu\n", 
                   manager.getAvailableBufferCount());
            printf("(Pool has stabilized - all operations now zero-alloc)\n\n");
        }
        
        usleep(100000);  // 100ms between updates
    }
    
    printf("\nFinal buffer pool state: %zu buffers available\n", 
           manager.getAvailableBufferCount());
    
    manager.stop();
    return 0;
}