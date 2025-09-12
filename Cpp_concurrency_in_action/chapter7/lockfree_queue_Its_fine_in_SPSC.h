#include <memory>
#include <atomic>

template<typename T>
class lock_free_queue
{
private:
    struct node{
        std::shared_ptr<T> data_;
        node* next_;
        node() : next_(nullptr) {}
    };

    std::atomic<node*> head_;
    std::atomic<node*> tail_;
    node* pop_head()
    {
        node* const old_head = head_.load();
        if(old_head == tail_.load()){
            return nullptr;
        }
        head_.store(old_head->next_);
        return old_head;
    }
};