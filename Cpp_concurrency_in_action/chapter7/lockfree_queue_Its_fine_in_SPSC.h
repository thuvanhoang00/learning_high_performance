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

    void push(T new_value){
        std::shared_ptr<T> new_data(std::make_shared<T>(new_value));
        node* p = new node;
        node* const old_tail = tail_.load();
        old_tail->data_.swap(new_data);
        old_tail->next_ = p;
        tail_.store(p);
    }
};