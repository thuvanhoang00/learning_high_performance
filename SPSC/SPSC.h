#ifndef SPSC_H
#define SPSC_H
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <condition_variable>

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
        for (int idx = 0; idx < size; idx++)
        {
            std::lock_guard<std::mutex> plk(mtx);
            q.push(idx);
            // std::cout << "Produce: " << idx << std::endl;
        }

        // {
        //     std::lock_guard<std::mutex> guard(producedAllMtx);
        //     producedAll = true;
        // }
    }

    void consume_mtx(){
        while (true)
        {
            std::lock_guard<std::mutex> lk(mtx);
            if (!q.empty())
            {
                int val = q.front();
                q.pop();
                // std::cout << "Consume: " << val << std::endl;
                consume_count++;
            }
            if(consume_count == size) break;
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
};

#endif