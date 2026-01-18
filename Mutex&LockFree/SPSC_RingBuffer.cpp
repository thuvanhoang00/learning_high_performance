#include <vector>
#include <atomic>
#include <array>
#include <thread>
#include <iostream>
#include <memory>
#include <cassert>

struct Point{
    int x;
    int y;
};

constexpr int N = 1440*1080*3;

struct Mat{
private:
    std::vector<Point> frame_;
public:
    Mat() : frame_(N, {1,2}) {}

    uint8_t at() const {
        return frame_[0].x;
    }
};




template<size_t N>
class RingBuffer{
private:
    struct Slot{
        Mat frame;
        std::atomic<uint64_t> seq{0};
    };

    std::array<Slot, N> slots_;
    std::atomic<uint64_t> write_seq_{0};
public:
    RingBuffer() = default;
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    void publish(Mat&& frame){
        auto seq = write_seq_.load(std::memory_order_relaxed);
        Slot& slot = slots_[seq%N];

        slot.frame = std::move(frame);
        slot.seq.store(seq+1, std::memory_order_release);
        write_seq_.store(seq+1, std::memory_order_release);
    }

    const Mat* latest() const{
        auto seq = write_seq_.load(std::memory_order_acquire);
        if(seq == 0) return nullptr;

        const Slot& slot = slots_[(seq-1)%N];
        if(slot.seq.load(std::memory_order_acquire) != seq) return nullptr;

        return &slot.frame;
    }
};


void test_concurrent() {
    RingBuffer<8> ring;
    std::atomic<bool> running{true};

    // Producer
    std::thread producer([&] {
        for (int i = 1; i <= 10; ++i) {
            Mat f;
            ring.publish(std::move(f));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        running.store(false);
    });

    // Consumer
    std::thread consumer([&] {
        while (running.load()) {
            if (const Mat* f = ring.latest()) {
                uint8_t v = f->at();
                assert(v==1);
            }
        }
    });

    producer.join();
    consumer.join();

    std::cout << "Concurrent test passed\n";
}


int main(){
    test_concurrent();
    return 0;
}