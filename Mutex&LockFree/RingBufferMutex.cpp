#include <mutex>
#include <vector>
#include <memory>

template<typename T>
class RingBuffer{
private:
    std::vector<T> buffer_;
    size_t max_size_;
    size_t head_;
    size_t tail_;
    bool full_;
    std::mutex mtx_;
public:
    RingBuffer(size_t size) : buffer_(size), max_size_(size), head_(0), tail_(0) {}

    // Leaky push: If full, overwrites the oldest data(head)
    void push_overwrite(T item){
        std::lock_guard<std::mutex> lk(mtx_);

        buffer_[tail_] = item;

        if(full_){
            head_ = (head_ + 1) % max_size_;
        }

        tail_ = (tail_ + 1) % max_size_;
        full_ = (tail_ == head_);
    }

    bool pop(T& item){
        std::lock_guard<std::mutex> lk(mtx_);

        if(empty()){
            return false;
        }

        item = buffer_[head_];
        head_ = (head_ + 1) % max_size_;
        full_ = false;

        return true;
    }

    bool empty() const{
        return (!full_ && (head_ == tail_));
    }
};