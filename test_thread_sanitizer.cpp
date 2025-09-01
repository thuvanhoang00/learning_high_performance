#include <memory>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include <random>
/*
* TSan can detect Datarace, deadlock, thread_leak...
*/


// Use TSan to detect deadlock
int shared_data=1;
std::mutex m1;
std::mutex m2;
void thread1(){
    m1.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m2.lock();
    shared_data++;
    m1.unlock();
    m2.unlock();
}
void thread2(){
    m2.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m1.lock();
    shared_data++;
    m2.unlock();
    m1.unlock();
}

// Compile with -fsanitize=thread option
int main(){
    std::thread t1(thread1);
    std::thread t2(thread2);
    t1.join();
    t2.join();
    return 0;
}