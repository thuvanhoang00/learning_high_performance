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

template <typename T>
class blocking_queue : protected std::queue<T> {
public:
    using wlock = std::unique_lock<std::shared_mutex>;
    using rlock = std::shared_lock<std::shared_mutex>;

public:
    blocking_queue() = default;
    ~blocking_queue(){
        clear();
    }
    blocking_queue(const blocking_queue&) = delete;  
    blocking_queue(blocking_queue&&) = delete; 
    blocking_queue& operator=(const blocking_queue&) = delete; 
    blocking_queue& operator=(blocking_queue&&) = delete; 
public:
    bool empty() const{
        rlock lock(mtx_);
        return std::queue<T>::empty();
    }

    size_t size() const{
        rlock lock(mtx_);
        return std::queue<T>::size();
    }

    void clear(){
        wlock lock(mtx_);
        while(!std::queue<T>::empty()){
            std::queue<T>::pop();
        }
    }

    void push(const T& obj){
        wlock lock(mtx_);
        std::queue<T>::push(obj);
    }

    template<typename... Args>
    void emplace(Args&&... args){
        wlock lock(mtx_);
        std::queue<T>::emplace(std::forward<Args>(args)...);
    }

    bool pop(T& holder){
        wlock lock(mtx_);
        if(std::queue<T>::empty()){
            return false;
        }
        else{
            holder = std::move(std::queue<T>::front());
            std::queue<T>::pop();
            return true;
        }
    }
private:   
    mutable std::shared_mutex mtx_;
};

class threadpool{
public:
    using wlock = std::unique_lock<std::shared_mutex>;
    using rlock = std::shared_lock<std::shared_mutex>;
public:
    threadpool() = default;
    ~threadpool(){
        terminate();
    }
    threadpool(const threadpool&) = delete;
    threadpool(threadpool&&) = delete;
    threadpool& operator=(const threadpool&) = delete;
    threadpool& operator=(threadpool&&) = delete;
public:
    void init(int num);
    void terminate();
    void cancel();
    bool inited() const;
    bool is_running() const;
    int size() const;
public:
    template<class F, class... Args>
    auto async(F&& f, Args&&... args) const->std::future<decltype(f(args...))>;

private:
    bool _is_running() const{
        return inited_ && !stop_ && !cancel_;
    }
    void spawn();

private:
    bool inited_{false};
    bool stop_{false};
    bool cancel_{false};
    
    mutable std::shared_mutex mtx_;
    mutable std::condition_variable_any cond_;
    mutable std::once_flag once_;

    std::vector<std::thread> workers_;
    mutable blocking_queue<std::function<void()>> tasks_;
};

inline void threadpool::init(int num){
    std::call_once(once_, [this, num](){
        wlock lock(mtx_);
        stop_ = false;
        cancel_ = false;
        workers_.reserve(num);
        for(int i=0; i<num; ++i){
            workers_.emplace_back(std::bind(&threadpool::spawn, this));
        }
        inited_ = true;
    });
}

inline void threadpool::terminate(){
    {
        wlock lock(mtx_);
        if(_is_running()){
            stop_ = true;
        }
        else{
            return ;
        }
    }

    cond_.notify_all();
    for(auto& worker : workers_){
        worker.join();
    }
}

inline void threadpool::cancel(){
    {
        wlock lock(mtx_);
        if(_is_running()){
            cancel_ = true;
        }
        else{
            return;
        }
    }

    tasks_.clear();
    cond_.notify_all();
    for(auto& worker : workers_){
        worker.join();
    }
}

inline bool threadpool::inited() const{
    rlock lock(mtx_);
    return inited_;
}

inline bool threadpool::is_running() const{
    rlock lock(mtx_);
    return _is_running();
}

inline int threadpool::size() const{
    rlock lock(mtx_);
    return workers_.size();
}

inline void threadpool::spawn(){
    for(;;){
        bool pop = false;
        std::function<void()> task;
        {
            wlock lock(mtx_);
            cond_.wait(lock, [this, &pop, &task](){
                pop = tasks_.pop(task);
                return cancel_ || stop_ || pop;
            });
        }

        if(cancel_ || (stop_ && !pop)){
            return;
        }
        task();
    }
}

template<class F, class... Args>
auto threadpool::async(F&& f, Args&&... args) const -> std::future<decltype(f(args...))>{
    using return_t = decltype(f(args...));
    using future_t = std::future<return_t>;
    using task_t = std::packaged_task<return_t()>;

    {
        rlock lock(mtx_);
        if(stop_ || cancel_){
            throw std::runtime_error("Delegating task to a threadpool that has been terminated or canceled");
        }
    }
    auto bind_func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    std::shared_ptr<task_t> task = std::make_shared<task_t>(std::move(bind_func));
    future_t fut = task->get_future();
    tasks_.emplace([task]()->void{
        (*task)();
    });
    cond_.notify_one();
    return fut;
}




int main()
{
    threadpool pool;
    pool.init(3);
    auto foo1 = [](){ return 100;};
    auto foo2 = [](){ return 100;};
    auto foo3 = [](){ return 100;};
    auto foo4 = [](){ return 100;};
    
    auto f1 = pool.async(foo1);
    auto f2 = pool.async(foo2);
    auto f3 = pool.async(foo3);
    auto f4 = pool.async(foo4);

    assert(f1.get()==100);
    assert(f2.get()==100);
    assert(f3.get()==100);
    assert(f4.get()==100);
    return 0;
}