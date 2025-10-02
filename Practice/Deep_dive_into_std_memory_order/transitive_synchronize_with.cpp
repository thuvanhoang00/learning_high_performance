#include <atomic>
#include <thread>
#include <cassert>

std::atomic<int> data[5];

#ifdef VER1
std::atomic<bool> sync1(false), sync2(false);

void thread1(){
    data[0].store(1, std::memory_order_relaxed);
    data[1].store(2, std::memory_order_relaxed);
    data[2].store(3, std::memory_order_relaxed);
    data[3].store(4, std::memory_order_relaxed);
    data[4].store(5, std::memory_order_relaxed);
    sync1.store(true, std::memory_order_release);
}

void thread2(){
    while(!sync1.load(std::memory_order_acquire));
    sync2.store(true, std::memory_order_release);
}


void thread3(){
    while(!sync2.load(std::memory_order_acquire));
    assert(data[0].load(std::memory_order_relaxed) == 1);
    assert(data[1].load(std::memory_order_relaxed) == 2);
    assert(data[2].load(std::memory_order_relaxed) == 3);
    assert(data[3].load(std::memory_order_relaxed) == 4);
    assert(data[4].load(std::memory_order_relaxed) == 5);
}
#endif

std::atomic<int> sync{0};

void thread1(){
    data[0].store(1, std::memory_order_relaxed);
    data[1].store(2, std::memory_order_relaxed);
    data[2].store(3, std::memory_order_relaxed);
    data[3].store(4, std::memory_order_relaxed);
    data[4].store(5, std::memory_order_relaxed);
    sync.store(1, std::memory_order_release);
}

void thread2(){
    int expected = 1;
    while(!sync.compare_exchange_strong(expected, 2, std::memory_order_acq_rel)){
        expected = 1;
    }
}


void thread3(){
    while(sync.load(std::memory_order_acquire) < 2);
    assert(data[0].load(std::memory_order_relaxed) == 1);
    assert(data[1].load(std::memory_order_relaxed) == 2);
    assert(data[2].load(std::memory_order_relaxed) == 3);
    assert(data[3].load(std::memory_order_relaxed) == 4);
    assert(data[4].load(std::memory_order_relaxed) == 5);
}


int main(){
    std::thread threads[3];
        threads[0] = std::move(std::thread(thread1));
        threads[1] = std::move(std::thread(thread2));
        threads[2] = std::move(std::thread(thread3));

    threads[0].join();
    threads[1].join();
    threads[2].join();


    return 0;
}