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
#include <unordered_map>

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


/**
 * Converts uint8_t to float and normalizes to [0.0, 1.0]
 * Uses flat iteration for maximum cache locality and auto-vectorization.
 */
void convert_to_float_and_normalize(const MockMat<uint8_t>& input, MockMat<float>& output) {
    // 1. Safety Checks (Crucial in both ADAS and Quant to prevent silent memory corruption)
    assert(input.rows == output.rows && input.cols == output.cols);
    assert(input.channels == output.channels);

    // 2. Pre-calculate constants
    // Multiplication is significantly faster than division on modern CPUs.
    // constexpr ensures this is evaluated at compile time.
    constexpr float INV_255 = 1.0f / 255.0f;

    // 3. Get raw pointers to the underlying contiguous memory
    const size_t total_elements = input.total();
    const uint8_t* src_ptr = input.data.data();
    float* dst_ptr = output.data.data();

    // 4. Flat 1D Iteration
    // The compiler loves this. It's perfectly predictable and easy to unroll.
    for (size_t i = 0; i < total_elements; ++i) {
        dst_ptr[i] = static_cast<float>(src_ptr[i]) * INV_255;
    }
}


/**
 * Converts uint8_t to float and normalizes to [0.0, 1.0]
 * Uses flat iteration for maximum cache locality and auto-vectorization.
 */
MockMat<float> convert_to_float_normalize_and_return(const MockMat<uint8_t>& input, MockMat<float>& output) {
    // 1. Safety Checks (Crucial in both ADAS and Quant to prevent silent memory corruption)
    assert(input.rows == output.rows && input.cols == output.cols);
    assert(input.channels == output.channels);

    // 2. Pre-calculate constants
    // Multiplication is significantly faster than division on modern CPUs.
    // constexpr ensures this is evaluated at compile time.
    constexpr float INV_255 = 1.0f / 255.0f;

    // 3. Get raw pointers to the underlying contiguous memory
    const size_t total_elements = input.total();
    const uint8_t* src_ptr = input.data.data();
    float* dst_ptr = output.data.data();

    // 4. Flat 1D Iteration
    // The compiler loves this. It's perfectly predictable and easy to unroll.
    for (size_t i = 0; i < total_elements; ++i) {
        dst_ptr[i] = static_cast<float>(src_ptr[i]) * INV_255;
    }

    return output;
}

// ----------------------------------------------
// =========================================================
// 1. SNPE API Mocks
// =========================================================

// Simulates zdl::DlSystem::ITensor
class ITensor {
public:
    std::vector<float> engine_buffer;

    // Standard ITensor creation usually copies the data
    ITensor(const float* input_data, size_t size) {
        // This is a deliberate copy to simulate engine overhead
        engine_buffer.assign(input_data, input_data + size);
    }
    
    // Simulates getting an output buffer back
    float* get_data() { return engine_buffer.data(); }
};

// Simulates zdl::DlSystem::StringList & ITensorMap
class ITensorMap {
private:
    std::unordered_map<std::string, ITensor*> tensor_map;

public:
    void add(const std::string& name, ITensor* tensor) {
        tensor_map[name] = tensor;
    }

    ITensor* getTensor(const std::string& name) const {
        auto it = tensor_map.find(name);
        return it != tensor_map.end() ? it->second : nullptr;
    }

    // RAII Cleanup for our mock
    ~ITensorMap() {
        for (auto& pair : tensor_map) {
            delete pair.second;
        }
    }
};

// Simulates zdl::SNPE::SNPE
class SNPE {
public:
    // The main execution call
    bool execute(const ITensorMap& input, ITensorMap& output) {
        // Simulate inference latency (preventing compiler optimization)
        volatile float dummy_compute = 0.0f;
        for (int i = 0; i < 100000; ++i) {
            dummy_compute = dummy_compute + 0.001f;
        }
        
        // Simulate writing to an output tensor (e.g., bounding boxes or classifications)
        // In a real scenario, SNPE allocates this output tensor.
        ITensor* out_tensor = new ITensor(input.getTensor("image_tensor:0")->get_data(), 100); // 100 float output
        output.add("Postprocessor/Decode", out_tensor);

        return true;
    }
};

// =========================================================
// 2. The Final Pipeline Step
// =========================================================

void run_inference(SNPE& snpe, const MockMat<float>& float_frame) {
    // 1. Create Input Tensor (Warning: This triggers a memory copy!)
    ITensor* input_tensor = new ITensor(float_frame.data.data(), float_frame.total());

    // 2. Map Tensors to Input/Output Nodes
    ITensorMap input_map;
    input_map.add("image_tensor:0", input_tensor); // Name matches your network's input layer

    ITensorMap output_map;

    // 3. Execute Neural Network
    // std::cout << "Executing SNPE..." << std::endl;
    bool success = snpe.execute(input_map, output_map);

    if (success) {
        // 4. Retrieve Results
        ITensor* result = output_map.getTensor("Postprocessor/Decode");
        if (result) {
            // Processing output...
            volatile float first_val = result->get_data()[0];
            (void)first_val;
        }
    }
}

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
    // Initialize our mocked Neural Engine
    SNPE my_snpe_engine;

    // 2. The Hot Loop
    while (keep_running.load(std::memory_order_relaxed)) {

        // Fill dummy data
        for (size_t i = 0; i < input.data.size(); ++i)
            input.data[i] = i % 255;
        
        // Resizing
        resize_frame(input, output);

        // MockMat<float> float_tensor(640, 640, 3);

        // Floating
        // convert_to_float_and_normalize(output, float_tensor);

        // Executing
        // run_inference(my_snpe_engine, float_tensor);

        // Update counter (relaxed memory order is fastest)
        total_frames.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
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