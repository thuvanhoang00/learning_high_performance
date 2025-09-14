#include <thread>
#include <future>
#include <thread>
#include <atomic>
#include <condition_variable>
class interrupt_flag
{
private:
    std::atomic<bool> flag_;
    std::condition_variable* thread_cond_;
    std::condition_variable_any* thread_cond_any_;
    std::mutex set_clear_mutex_;
public:
    interrupt_flag() : thread_cond_(0) {}
    void set(){
        flag_.store(true, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(set_clear_mutex_);
        if(thread_cond_){
            thread_cond_->notify_all();
        }
        else if(thread_cond_any_){
            thread_cond_any_->notify_all();
        }
    }

    template<typename Lockable>
    void wait(std::condition_variable_any& cv, Lockable& lk)
    {
        struct custom_lock{
            interrupt_flag* self_;
            Lockable& lk_;
            custom_lock(interrupt_flag* self, std::condition_variable_any& cv, Lockable& lk)
                : self_(self), lk_(lk)
            {
                self_->set_clear_mutex_.lock();
                self_->thread_cond_any_ = &cv;
            }
            void unlock()
            {
                lk_.unlock();
                self_->set_clear_mutex_.unlock();
            }
            void lock()
            {
                std::lock(self_->set_clear_mutex_, lk_);
            }
            ~custom_lock(){
                self_->thread_cond_any_ = 0;
                self_->set_clear_mutex_.unlock();
            }
        };
        custom_lock cl(this, cv, lk);
        interruption_point();
        cv.wait(cl);
        interruption_point();
    }

    bool is_set() const{
        return flag_.load(std::memory_order_relaxed);
    }

    void set_condition_variable(std::condition_variable& cv){
        std::lock_guard<std::mutex> lk(set_clear_mutex_);
        thread_cond_ = &cv;
    }
    void clear_condition_variable(){
        std::lock_guard<std::mutex> lk(set_clear_mutex_);
        thread_cond_ = 0;
    }

    struct clear_cv_on_destruct
    {
        ~clear_cv_on_destruct(){
            this_thread_interrupt_flag.clear_condition_variable();
        }
    };
};

thread_local interrupt_flag this_thread_interrupt_flag;

class interruptible_thread
{
private:
    std::thread internal_thread_;
    interrupt_flag* flag_;
public:
    template<typename FunctionType>
    interruptible_thread(FunctionType f){
        std::promise<internal_thread*> p;
        internal_thread = std::thread([f, &p](){
            p.set_value(&this_thread_interrupt_flag);
            f();
        });
        flag_ = p.get_future().get();
    }
    void join();
    void detach();
    bool joinable() const;
    void interrupt(){
        if(flag_){
            flag_->set();
        }
    }
};

void interruption_point()
{
    if(this_thread_interrupt_flag.is_set()){
        // throw thread_interrupted();
    }
}

void interruptible_wait(std::condition_variable& cv, std::unique_lock<std::mutex>& lk){
    interruption_point();
    this_thread_interrupt_flag.set_condition_variable(cv);
    interrupt_flag::clear_cv_on_destruct guard;
    interruption_point();
    cv.wait_for(lk, std::chrono::milliseconds(1));
    interruption_point();
}

template<typename Predicate>
void interruptible_wait(std::contidion_variable& cv, std::unique_lock<std::mutex>& lk, Predicate pred)
{
    interruption_point();
    this_thread_interrupt_flag.set_condition_variable(cv);
    interrupt_flag::clear_cv_on_destruct guard;
    while(!this_thread_interrupt_flag.is_set() && !pread()){
        cv.wait_for(lk, std::chrono::milliseconds(1));
    }
    interruption_point();
}

template<typename Lockable>
void interruptible_wait(std::condition_variabale_any& cv, Lockable& lk)
{
    this_thread_interrupt_flag.wait(cv, lk);
}

template<typename T>
void interruptible_wait(std::future<T>& uf)
{
    while(!this_thread_interrupt_flag.is_set()){
        if(uf.wait_for(lk, std::chrono::milliseconds(1)) == std::future_status::ready){
            break;
        }
    }
    interruption_point();
}
//
