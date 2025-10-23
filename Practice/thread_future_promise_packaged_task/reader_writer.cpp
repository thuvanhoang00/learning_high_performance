#include <shared_mutex>
#include <thread>
#include <mutex>
#include <vector>
#include <iostream>

std::shared_mutex shmtx_;
int data=0;

void read()
{
    while(true)
    {
        {
            std::shared_lock<std::shared_mutex> lk(shmtx_);
            std::cout << "ThreadID: " << std::this_thread::get_id() << " - read: " << data << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void write()
{
    while (true)
    {
        {
            std::unique_lock<std::shared_mutex> lk(shmtx_);
            ++data;
            std::cout << "ThreadID: " << std::this_thread::get_id() << " - write: " << data << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

int main(){
    std::vector<std::thread> r(4);
    for(auto& t : r){
        t = std::move(std::thread(read));
    }
    std::thread w(write);
    
    for(auto& t :r){
        t.join();
    }
    w.join();
    return 0;
}