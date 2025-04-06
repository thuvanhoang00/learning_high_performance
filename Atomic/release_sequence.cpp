#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <cassert>

std::vector<int> data;
std::atomic<int> flag{0};


void thread1(){
    data.push_back(42); // 1
    flag.store(1, std::memory_order_release); // 2
}

void thread2(){
    int expected=1; // 3
    while(!flag.compare_exchange_strong(expected, 2, std::memory_order_relaxed)) // 4: is not synchronizes-with (due to releaxed here) 2 but is "release-sequence" with 2
        expected = 1;
}

void thread3(){
    while(flag.load(std::memory_order_acquire) < 2); // 5 because of 4 is release-sequence with 2 then 5 is synchronizes-with 2
    
    assert(data[0]==42); // 6    
}

int main()
{
    std::thread t1(thread1);
    std::thread t2(thread2);
    std::thread t3(thread3);
    t1.join();
    t2.join();
    t3.join();
    return 0;
}