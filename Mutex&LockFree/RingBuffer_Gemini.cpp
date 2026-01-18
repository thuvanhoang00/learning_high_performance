#include <opencv2/opencv.hpp>
#include <iostream>
#include <atomic>
#include <vector>
#include <memory>
#include <thread>

// ---------------------------------------------------------
// 1. THE OBJECT POOL (Where the 10MB is actually allocated)
// ---------------------------------------------------------
class FramePool {
private:
    std::vector<cv::Mat> storage;
    std::vector<cv::Mat*> available;
    std::mutex mtx;

public:
    FramePool(size_t count, int rows, int cols, int type) {
        storage.resize(count);
        for (size_t i = 0; i < count; ++i) {
            // ALLOCATION HAPPENS HERE (Once at startup)
            storage[i] = cv::Mat(rows, cols, type);
            available.push_back(&storage[i]);
        }
    }

    // Get a frame wrapped in a shared_ptr with a Custom Deleter
    std::shared_ptr<cv::Mat> acquire() {
        std::lock_guard<std::mutex> lock(mtx);
        if (available.empty()) return nullptr;

        cv::Mat* raw = available.back();
        available.pop_back();

        // When this shared_ptr's ref-count hits 0, it returns to the pool
        return std::shared_ptr<cv::Mat>(raw, [this](cv::Mat* p) {
            std::lock_guard<std::mutex> returnLock(this->mtx);
            this->available.push_back(p);
        });
    }
};

// ---------------------------------------------------------
// 2. THE FRESHNESS RING BUFFER (The Signal Lane)
// ---------------------------------------------------------
template<typename T, size_t Capacity = 8>
class FreshnessRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
private:
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    T buffer_[Capacity]; // Array of shared_ptr<cv::Mat>
    const size_t mask_ = Capacity - 1;

public:
    // PRODUCER: Always pushes. Overwrites oldest if full.
    void push_latest(T item) {
        size_t t = tail_.load(std::memory_order_relaxed);
        size_t h = head_.load(std::memory_order_acquire);

        // If buffer is full, advance head to drop the oldest frame
        if (((t + 1) & mask_) == h) {
            head_.store((h + 1) & mask_, std::memory_order_release);
            // The old item at buffer_[h] is overwritten, 
            // decreasing its ref count automatically.
        }

        buffer_[t] = std::move(item);
        tail_.store((t + 1) & mask_, std::memory_order_release);
    }

    // CONSUMER: Grabs the oldest available frame
    bool try_pop(T& out_item) {
        size_t h = head_.load(std::memory_order_relaxed);
        size_t t = tail_.load(std::memory_order_acquire);

        if (h == t) return false; // Empty

        out_item = buffer_[h];
        head_.store((h + 1) & mask_, std::memory_order_release);
        return true;
    }
};

// ---------------------------------------------------------
// 3. USAGE IN ADAS PIPELINE
// ---------------------------------------------------------

// Define our frame type
using SharedFrame = std::shared_ptr<cv::Mat>;

// Global or Class-scoped buffers for each algorithm
FreshnessRingBuffer<SharedFrame, 4> laneBuffer;
FreshnessRingBuffer<SharedFrame, 4> objectBuffer;

void producerThread(FramePool& pool) {
    cv::VideoCapture cap(0);
    while (true) {
        SharedFrame frame = pool.acquire();
        if (!frame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        cap >> *frame; // Write data into the pre-allocated memory
        if (frame->empty()) break;

        // Push the same pointer to all algorithms
        laneBuffer.push_latest(frame);
        objectBuffer.push_latest(frame);
        
        // 5 FPS = 200ms
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void laneDetectionConsumer() {
    while (true) {
        SharedFrame frame;
        if (laneBuffer.try_pop(frame)) {
            // DO WORK
            // cv::imshow("Lanes", *frame);
            std::cout << "Processing Lanes on frame: " << frame.get() << std::endl;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

int main() {
    // 1. Allocate 15 frames of 10MB each (1080p BGR is ~6MB, 4K is ~25MB)
    // Adjust 1920, 1080 to your actual 10MB resolution
    FramePool pool(15, 1080, 1920, CV_8UC3);

    std::thread p(producerThread, std::ref(pool));
    std::thread c1(laneDetectionConsumer);
    // ... start other consumers

    p.join();
    c1.join();
    return 0;
}