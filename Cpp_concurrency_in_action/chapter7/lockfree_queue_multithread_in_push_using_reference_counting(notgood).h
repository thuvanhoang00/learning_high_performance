#include <memory>
#include <atomic>

template<typename T>
class lock_free_queue
{
private:
    struct node;
    
    struct counted_node_ptr{
        int external_count_;
        node* ptr_;
    };
    
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

public:
    lock_free_queue() : head_(new node), tail_(head_.load()) {}
    lock_free_queue(const lock_free_queue&) = delete;
    lock_free_queue& opeartor=(const lock_free_queue&) = delete;
    ~lock_free_queue(){
        while(node* const old_head = head_.load()){
            head_.store(old_head->next_);
            delete old_head;
        }
    }
    std::shared_ptr<T> pop()
    {
        node* old_head = pop_head();
        if(!old_head){
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> const res(old_head->data_);
        delete old_head;
        return res;
    }

    // Multi threads in push
    void push(T new_value){
        std::unique_ptr<T> new_data(new T(new_value));
        counted_node_ptr new_next;
        new_next.ptr_ = new node;
        new_next.external_count_ = 1;
        for(;;){
            node* const old_tail = tail_.load();
            T* old_data = nullptr;
            if(old_tail->data_.compare_exchange_strong(old_data, new_data.get())){
                old_tail->next_ = new_next;
                tail_.store(new_next.ptr_);
                new_data.release();
                break;
            }
        }
    }
};