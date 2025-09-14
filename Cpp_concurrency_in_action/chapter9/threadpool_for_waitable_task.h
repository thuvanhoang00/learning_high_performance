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
    threadsafe_queue<function_wrapper> work_queue_;
    std::vector<std::thread> threads_;
    join_threads joiner_;
    void worker_thread()
    {
        while(!done_){
            function_wrapper task;
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
        work_queue_.push(std::move(task));
        return res;
    }
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
    unsigned long const length = std::distance(first, last);
    if(!length) return init;

    unsigned long const block_size = 25;
    unsigned long const num_blocks = (length+block_size-1)/block_size;
    std::vector<std::future<T>> futures(num_blocks-1);
    thread_pool pool;
    Iterator block_start = first;
    for(unsigned long i = 0; i<(num_blocks-1); ++i){
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        futures[i]=pool.submit([=](){accumulate_block<Iterator, T>()(block_start, block_end);});
        block_start = block_end;
    }   
    T last_result = accumulate_block<Iterator, T>(){block_start, last};
    T result = init;
    for(unsigned long i=0; i<(num_blocks-1); ++i){
        result += futures[i].get();
    }
    result += last_result;
    return result;
}