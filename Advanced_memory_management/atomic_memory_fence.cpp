#include <atomic>
#include <iostream>
#include <thread>

int a = 0, b = 0;
std::atomic<bool> ready{false};
void write_a_then_b(){
    a = 1;
    std::atomic_thread_fence(std::memory_order_release);
    b = 1;
}

void read_b_then_a(){
    while(!ready.load(std::memory_order_acquire));
    std::cout << "b: " << b << ", a: " << a << std::endl;
}

int main(){
    std::thread w(write_a_then_b);
    std::thread r(read_b_then_a);
    ready.store(true, std::memory_order_release);
    w.join();
    r.join();
    return 0;
}