#include "threadsafe_queue.h"
#include <functional>

class join_threads
{
private:
    std::vector<std::thread>& threads_;
public:
    explicit join_threads(std::vector<std::thread>& threads) : threads_(threads) {}
    ~join_threads(){
        for(unsigned long i=0; i<threads_.size(); ++i){
            if(threads_[i].joinable()){
                threads_[i].join();
            }
        }
    }
};

class thread_pool
{
private:
    std::atomic_bool done_;
    threadsafe_queue<std::function<void()>> work_queue_;
    std::vector<std::thread> threads_;
    join_threads joiner_;
    void worker_thread()
    {
        while(!done_){
            std::function<void()> task;
            if(work_queue_.try_pop(task)){
                task();
            }
            else{
                std::this_thread::yield();
            }
        }
    }
public:
    thread_pool() : done_(false), joiner_(threads_)
    {
        unsigned const thread_count = std::thread::hardware_concurrency();
        try{
            for(unsigned i=0; i<thread_count; ++i){
                threads_.push_back(std::thread(&thread_pool::worker_thread, this));
            }
        }
        catch(...)
        {
            done_ = true;
            throw;
        }
    }
    
    ~thread_pool(){
        done_ = true;
    }

    template<typename FunctionType>
    void submit(FunctionType f){
        work_queue_.push(std::function<void()>(f));
    }
};