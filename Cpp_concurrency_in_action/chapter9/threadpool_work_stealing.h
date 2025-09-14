#include "threadsafe_queue.h"
#include <functional>
#include <future>
#include <algorithm>
#include <queue>
#include <deque>

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

class work_stealing_queue
{
private:
    typedef function_wrapper data_type;
    std::deque<data_type> the_queue_;
    mutable std::mutex the_mutex_;
public:
    work_stealing_queue(){}
    work_stealing_queue(const work_stealing_queue&) = delete;
    work_stealing_queue& operator=(const work_stealing_queue&) = delete;
    void push(data_type data){
        std::lock_guard<std::mutex> lock(the_mutex_);
        the_queue_.push_front(std::move(data));
    }
    bool empty() const{
        std::lock_guard<std::mutex> lock(the_mutex_);
        return the_queue_.empty();
    }
    bool try_pop(data_type& res){
        std::lock_guard<std::mutex> lock(the_mutex_);
        if(the_queue_.empty()){
            return false;
        }
        res = std::move(the_queue_.front());
        the_queue_.pop_front();
        return true;
    }
    bool try_steal(data_type& res){ //try steal takes items from opposite end of the queue to try pop -> minimize contention
        std::lock_guard<std::mutex> lock(the_mutex_);
        if(the_queue_.empty()){
            return false;
        }
        res = std::move(the_queue_.back());
        the_queue_.pop_back();
        return true;
    }
};


class thread_pool
{
private:
    typedef function_wrapper task_type;
    std::atomic_bool done_;
    threadsafe_queue<function_wrapper> pool_work_queue_;
    std::vector<std::unique_ptr<work_stealing_queue>> queues_;
    std::vector<std::thread> threads_;
    join_threads joiner_;
    
    static thread_local work_stealing_queue* local_work_queue_;
    static thread_local unsigned my_index_;
    void worker_thread(unsigned idx)
    {
        my_index_ = idx;
        local_work_queue_ = queues_[my_index_].get();
        while(!done_){
            run_pending_task();
        }
    }

    bool pop_task_form_local_queue(task_type& task){
        return local_work_queue_ && local_work_queue_->try_pop(task);
    }

    bool pop_task_from_pool_queue(task_type& task){
        return pool_work_queue_.try_pop(task);
    }

    bool pop_task_from_other_thread_queue(task_type& task){
        for(unsigned i=0; i<queues_.size(); ++i){
            unsigned const index = (my_index_ + i + 1)%queues_.size();
            if(queues_[index]->try_steal(task)){
                return true;
            }
        }
        return false;
    }
public:
    thread_pool() : done_(false), joiner_(threads_)
    {
        unsigned const thread_count = std::thread::hardware_concurrency();
        try{
            for(unsigned i=0; i<thread_count; ++i){
                queues_.push_back(std::unique_ptr<work_stealing_queue>(std::make_unique<work_stealing_queue>()));
            }

            for(unsigned i=0; i<thread_count; ++i){
                threads_.push_back(std::thread(&thread_pool::worker_thread, this, i));
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
        if(pop_task_form_local_queue(task) || pop_task_from_pool_queue(task) || pop_task_from_other_thread_queue(task)){
            task();
        }

        else{
            std::this_thread::yield();
        }
    }
};

