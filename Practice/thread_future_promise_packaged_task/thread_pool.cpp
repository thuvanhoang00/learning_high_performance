#include <thread>
#include <vector>
#include <queue>
#include <future>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <functional>

class thread_pool{
private:
    std::vector<std::thread> threads_;
    std::queue<std::packaged_task<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> done_;
public:
    thread_pool(int n=std::thread::hardware_concurrency()){
        done_ = false;
        // construct threads
        for(int i=0; i<n; ++i){
            threads_.push_back(std::thread(&thread_pool::worker, this));
        }
    }

    ~thread_pool(){
        done_ = true;
        cv_.notify_all();
        for(auto& t : threads_){
            if(t.joinable()) t.join();
        }
    }
private:
    void worker(){
        while(true){
            std::packaged_task<void()> task;
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this]{
                    return !tasks_.empty() || done_;
                });
                if(done_ && tasks_.empty()) return; // is it neccessary
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }
public:
    template<typename F, typename ...Args>
    // why universal reference here
    auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> { 
        using R = std::invoke_result_t<F, Args...>;
        // function binding
        auto bound = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        // create packaged task from bound function
        std::packaged_task<R()> task (std::move(bound));
        // take future
        std::future<R> fut = task.get_future();

        // emplace thunk to tasks_
        {
            std::lock_guard<std::mutex> lk(mtx_);
            tasks_.emplace([t = std::move(task)]() mutable {t();});
        }
        cv_.notify_one();
        return fut;
    }
};

int dosth(int n){
    return n+1;
}

int main(){
    thread_pool pool(10);
    auto f = pool.enqueue([](int a)->int{return a;}, 1);
    std::cout << f.get();

    return 0;
}