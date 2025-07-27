#include <mutex>
#include <shared_mutex>
#include <queue>
#include <vector>
#include <thread>
#include <functional>
#include <future>
#include <condition_variable>
#include <memory>
#include <cassert>
#include <stdexcept>
#include <utility>
#include <iostream>

namespace thu{
class SimpleThreadPool{
public:
    explicit SimpleThreadPool(size_t threads = std::thread::hardware_concurrency())
        : stop_(false)
    {
        if(threads == 0){
            threads == 1;
        }

        workers_.reserve(threads);
        for(size_t i = 0; i<threads; ++i){
            workers_.emplace_back([this]{
                for(;;){ // loop until receive a task from queue for running
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this]{
                            return stop_ || !tasks_.empty();
                        });
                        if(stop_ && tasks_.empty()){
                            return;
                        }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    // call task()
                    task();
                }
            });
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args... args)-> std::future<std::invoke_result_t<F, Args...>>
    {
        using return_t = std::invoke_result_t<F, Args...>;

        // create a task with packaged task and arguments
        auto task = std::make_shared<std::packaged_task<return_t()>>(
            [func=std::forward<F>(f), tup=std::make_tuple(std::forward<Args>(args)...)]()mutable{
                return std::apply(std::move(func), std::move(tup));
            }
        );

        std::future<return_t> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if(stop_){
                throw std::runtime_error("enqueue on stopped threadpool");
            }

            tasks_.emplace([task=std::move(task)](){
                (*task)();
            });
        }
        condition_.notify_one();
        return res;
    }

    void simple_enqueue(std::function<void()> task){
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if(stop_) throw std::runtime_error("enqueue on stopped threadpool");
            tasks_.emplace(task);
        }
        condition_.notify_one();
    }

private:
    bool stop_;
    std::vector<std::thread> workers_;
    std::mutex queue_mutex_;
    std::queue<std::function<void()>> tasks_;
    std::condition_variable condition_;
};
}

int main()
{
    thu::SimpleThreadPool pool(4);
    auto fut1 = pool.enqueue([](int a, int b){return a+b;}, 2,3);
    auto fut2 = pool.enqueue([]{return "hello!";});
    std::cout << "fut1: " << fut1.get() << std::endl;
    std::cout << "fut2: " << fut2.get() << std::endl;
    // for(int i=0; i<4; ++i){
    //     pool.simple_enqueue([i]{
    //         std::cout << "Task " << i << " is running on thread " << std::this_thread::get_id() << std::endl;
    //         std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //     });
    // }
    return 0;
}


