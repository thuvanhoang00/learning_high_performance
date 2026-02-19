/*
 * PURPOSE: 
 * Using perf to profile cache misses
 * Benchmarking and Optimizing MockMat
 * Using AI for coding and generating other things
*/

#include <vector>
#include <iostream>
#include <cassert>
#include <numeric> // for std::accumulate
#include <chrono>
#include <thread>
#include <atomic>

void pin_thread_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    
    pthread_t current_thread = pthread_self();
    if (pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "Error pinning thread to core " << core_id << std::endl;
    } else {
        std::cout << "📌 Thread pinned to Core " << core_id << std::endl;
    }
}

template <typename T>
class MockMat {
public:
    int rows;
    int cols;
    int channels; // New member
    
    std::vector<T> data;

    // Constructor defaults to 1 channel if not specified
    MockMat(int r, int c, int chan = 1, T initialValue = T()) 
        : rows(r), cols(c), channels(chan) {
        // Size = Width * Height * Channels
        data.resize(r * c * chan, initialValue);
    }

    // 1. Row Pointer (The most important function for performance)
    // Returns pointer to the start of the row
    T* ptr(int r) {
        // Stride = cols * channels
        return &data[r * cols * channels];
    }
    
    const T* ptr(int r) const {
        return &data[r * cols * channels];
    }

    // 2. Pixel Pointer (Get pointer to the specific pixel tuple)
    // Useful for accessing a specific (R,G,B) block
    T* ptr(int r, int c) {
        return &data[(r * cols + c) * channels];
    }

    // 3. Safe Accessor for specific channel (slower, for debugging)
    T& at(int r, int c, int chan_idx) {
        assert(chan_idx < channels);
        return data[(r * cols + c) * channels + chan_idx];
    }

    bool empty() const { return data.empty(); }
    size_t total() const { return data.size(); }
};
template <typename T>
void resize_frame(const MockMat<T>& input, MockMat<T>& output) {
    if (input.channels != output.channels) {
        std::cerr << "Error: Channel mismatch!" << std::endl;
        return;
    }

    float scale_x = (float)input.cols / output.cols;
    float scale_y = (float)input.rows / output.rows;
    int channels = input.channels;

    // Iterate over DESTINATION (Output) coordinates
    // This ensures contiguous writes (Cache Friendly)
    for (int dst_y = 0; dst_y < output.rows; ++dst_y) {
        
        // Map destination Y to source Y
        int src_y = (int)(dst_y * scale_y);
        if (src_y >= input.rows) src_y = input.rows - 1;

        // Get row pointers (Optimization: reduces multiplication in inner loop)
        const T* src_row_ptr = input.ptr(src_y);
        T* dst_row_ptr = output.ptr(dst_y);

        for (int dst_x = 0; dst_x < output.cols; ++dst_x) {
            
            // Map destination X to source X
            int src_x = (int)(dst_x * scale_x);
            if (src_x >= input.cols) src_x = input.cols - 1;

            // Calculate offsets
            int src_index = src_x * channels;
            int dst_index = dst_x * channels;

            // Copy all channels for this pixel
            for (int k = 0; k < channels; ++k) {
                dst_row_ptr[dst_index + k] = src_row_ptr[src_index + k];
            }
        }
    }
}

// ----------------------------------------------

// Shared Control Flags (Lock-Free)
std::atomic<bool> keep_running(true);
std::atomic<long long> total_frames(0);

// The Worker Function
// We pass by value (copy) to simulate thread-local storage, 
// but in reality, you'd likely pass a pointer to a shared buffer ring.
void worker_thread_func() {
    pin_thread_to_core(2);
    // 1. Name the thread for 'perf'
    // This is crucial: without this, perf report just says "adas_mock"
    pthread_setname_np(pthread_self(), "ADAS_Worker");

    // Setup Memory (Thread Local to avoid False Sharing)
    MockMat<uint8_t> input(800, 1280, 3);
    MockMat<uint8_t> output(640, 640, 3);
    
    // Fill dummy data
    for(size_t i=0; i<input.data.size(); ++i) input.data[i] = i % 255;

    std::cout << input.total() << " , " << output.total() << std::endl;
    // Warmup
    resize_frame(input, output);

    // 2. The Hot Loop
    while (keep_running.load(std::memory_order_relaxed)) {
        resize_frame(input, output);
        
        // Update counter (relaxed memory order is fastest)
        total_frames.fetch_add(1, std::memory_order_relaxed);
    }
}

int main() {
    std::cout << "🚀 Launching ADAS Worker Thread..." << std::endl;

    // 1. Spawn Thread
    std::thread worker(worker_thread_func);

    // 2. Main Thread Sleeps (Simulating UI or Control Logic)
    std::cout << "⏱️  Main thread sleeping for 60 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(60));

    // 3. Signal Stop
    std::cout << "🛑 Stopping worker..." << std::endl;
    keep_running.store(false);

    // 4. Join
    if (worker.joinable()) {
        worker.join();
    }

    std::cout << "✅ Finished. Total Frames: " << total_frames.load() << std::endl;
    std::cout << "   Average FPS: " << total_frames.load() / 60.0 << std::endl;

    return 0;
}