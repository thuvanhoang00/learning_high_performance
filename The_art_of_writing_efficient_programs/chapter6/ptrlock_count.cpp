template<typename T>
#include <atomic>
class PtrSpinlock{
public:
    explicit PtrSpinlock(T* p) : p_(p) {}
    T* lock(){
        while(!(saved_p_ = p_.exchange(nullptr, std::memory_order_acquire))){}
    }
    void unlock(){
        p_.store(save_p_, std::memory_order_release);
    }
private:
    std::atomic<T*> p_;
    T* saved_p_ = nullptr;

};