#ifndef SPSC_H
#define SPSC_H
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <time.h>
class SpinLock{
public:
    SpinLock() : flag_(0){}

    void lock(){
        static const timespec ns = {0, 1};
        for(int i=0; flag_.load(std::memory_order_relaxed) || flag_.exchange(1, std::memory_order_acquire); ++i){
            if(i==8){
                i=0;
                nanosleep(&ns, nullptr);
            }
        }
    }

    void unlock(){
        flag_.store(0, std::memory_order_release);
    }

private:
    std::atomic<unsigned int> flag_;
};

struct SPSC
{
    SPSC(int size_) : size (size_){}
    void produce_cv(){
        for (int idx = 0; idx < size; idx++)
        {
            std::unique_lock<std::mutex> plk(mtx);
            p_cv.wait(plk, [&]()
                      { return q.empty(); });
            q.push(idx);
            plk.unlock();
            c_cv.notify_one();

        }
        {
            std::lock_guard<std::mutex> guard(producedAllMtx);
            producedAll = true;
        }
        // have to notify here
        c_cv.notify_one();
    }
    void consume_cv(){
        while (true)
        {
            std::unique_lock<std::mutex> lk(mtx);
            c_cv.wait(lk, [&]()
                      { return !q.empty() || producedAll; });
            if (!q.empty())
            {
                int val = q.front();
                q.pop();
                lk.unlock();
                p_cv.notify_one();
            }
            else if (producedAll)
                break;
        }
    }

    void produce_mtx(){
        // std::cout << "MTX: " << size << std::endl;
        for (int idx = 0; idx < size; idx++)
        {
            std::lock_guard<std::mutex> plk(mtx);
            q.push(idx);
        }
    }

    void consume_mtx(){
        while (true)
        {
            std::lock_guard<std::mutex> lk(mtx);
            if (!q.empty())
            {
                int val = q.front();
                q.pop();
                consume_count++;
            }
            if(consume_count == size) break;
        }
    }

    void produce_sp(){
        // std::cout << "SP: " << size << std::endl;
        for(int i=0; i <size; i++){
            spin.lock();
            q.push(i);
            spin.unlock();
        }
    }
    void consume_sp(){
        while(consume_count<size){
            spin.lock();
            if(!q.empty()) {
                int val = q.front();
                q.pop();
                consume_count++;
            }
            spin.unlock();
        }
    }

    /*
     *   Spinlock version 2
     *   Using 1 atomic
     */
    void produce_sp2(){
        for(int i=0; i <size; i++){
            unsigned int expected = 0;
            while(!spin2.compare_exchange_weak(expected, 1, std::memory_order_acquire) || (expected==1));

            q.push(i);

            spin2.store(0, std::memory_order_release);
        }
    }
    void consume_sp2(){
        while(consume_count < size){
            unsigned int expected = 0;
            while(!spin2.compare_exchange_weak(expected, 1, std::memory_order_acquire) || (expected==1));

            if(!q.empty()) {
                int val = q.front();
                q.pop();
                consume_count++;
            }
            // if(consume_count == size) break;

            spin2.store(0, std::memory_order_release);
        }
    }

private:
    std::condition_variable p_cv;
    std::condition_variable c_cv;
    std::mutex mtx, producedAllMtx;
    bool producedAll{false};
    std::queue<int> q;
    int size;
    int consume_count=0;
    SpinLock spin;
    
    /*
    *   Spinlock version 2
    *   Using 1 atomic
    */
   std::atomic<unsigned int> spin2;
};

#endif