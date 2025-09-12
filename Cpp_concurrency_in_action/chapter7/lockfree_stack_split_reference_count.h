#include <atomic>
#include <memory>
template<typename T>
class lock_free_stack
{
private:
    struct node;
    struct counted_node_ptr{
        int external_count_;
        node* ptr_;
    };

    struct node{
        std::shared_ptr<T> data_;
        std::atomic<int> internal_count_;
        counted_node_ptr next_;
        node(T const& data) : data_(std::make_shared<T>(data)), internal_count_(0) {}        
    };
    std::atomic<counted_node_ptr> head_;

    void increase_head_count(counted_node_ptr& old_counter)
    {
        counted_node_ptr new_counter;
        do{
            new_counter = old_counter;
            ++new_counter.external_count_;
        }
        while(!head_.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));
        old_counter.external_count_ = new_counter.external_count_;
    }
public:
    void push(T const& data)
    {
        counted_node_ptr new_node;
        new_node.ptr_ = new node(data);
        new_node.external_count_ = 1;
        new_node.ptr_->next_ = head_.load(std::memory_order_relaxed);
        while(!head_.compare_exchange_weak(new_node.ptr_->next_, new_node, std::memory_order_release, std::memory_order_relaxed));
    }

    std::shared_ptr<T> pop()
    {
        counted_node_ptr old_head = head_.load(std::memory_order_relaxed);
        for(;;)
        {
            increase_head_count(old_head);
            node* const ptr = old_head.ptr_;
            if(!ptr){
                return std::shared_ptr<T>();
            }
            if(head_.compare_exchange_strong(old_head, ptr->next_, std::memory_order_relaxed)){
                std::shared_ptr<T> res;
                res.swap(ptr->data_);
                int const count_increase = old_head.external_count_-2;
                if(ptr->internal_count_.fetch_add(count_increase, std::memory_order_release) == -count_increase){
                    delete ptr;
                }
                return res;
            }
            else if(ptr->internal_count_.fetch_sub(1, std::memory_order_relaxed) == 1){
                delete ptr;
            }
        }
    }

    ~lock_free_stack()
    {
        while(pop());
    }
    
};