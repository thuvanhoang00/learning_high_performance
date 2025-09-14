#include "threadsafe_queue.h"
#include <functional>
#include <future>
#include <algorithm>

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

class function_wrapper
{
private:
    struct impl_base{
        virtual void call() = 0;
        virtual ~impl_base() {}
    };

    std::unique_ptr<impl_base> impl_;
    template<typename F>
    struct impl_type : impl_base
    {
        F f_;
        impl_type(F&& f) : f_(std::move(f)) {}
        void call() {f_();}
    };
public:
    template<typename F>
    function_wrapper(F&& f) : impl_(new impl_type<F>(std::move(f_))) {}
    void operator()() {impl_->call();}
    function_wrapper() = default;
    function_wrapper(function_wrapper&& other) : impl_(std::move(other.impl_)) {}
    function_wrapper& operator=(function_wrapper&& other)
    {
        impl_ = std::move(other.impl_);
        return *this;
    }
    function_wrapper(const function_wrapper&) = delete;
    function_wrapper& operator=(const function_wrapper&) = delete;
    // function_wrapper(function_wrapper&) = delete;
};

class thread_pool
{
private:
    std::atomic_bool done_;
    threadsafe_queue<function_wrapper> pool_work_queue_;
    typedef std::queue<function_wrapper> local_queue_type;
    static thread_local std::unique_ptr<local_queue_type> local_work_queue_;
    std::vector<std::thread> threads_;
    join_threads joiner_;
    void worker_thread()
    {
        local_work_queue_.reset(new local_queue_type);
        while(!done_){
            run_pending_task();
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

    // template<typename FunctionType>
    // void submit(FunctionType f){
    //     work_queue_.push(std::function<void()>(f));
    // }

    template<typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type>
    submit(FunctionType f)
    {
        typedef typename std::result_of<FunctionType()>::type result_type;
        std::packaged_task<result_type()> task(std::move(f));
        std::future<result_type> res(task.get_future());
        if(local_work_queue_){
            local_work_queue_->push(std::move(task));
        }
        else{
            pool_work_queue_.push(std::move(task));
        }
        return res;
    }

    void run_pending_task(){
        function_wrapper task;
        if(local_work_queue_ && !local_work_queue_->empty()){
            task = std::move(local_work_queue_->front());
            local_work_queue_->pop();
            task();
        }
        else if(pool_work_queue_.try_pop(task)){
            task();
        }
        else{
            std::this_thread::yield();
        }
    }
};

