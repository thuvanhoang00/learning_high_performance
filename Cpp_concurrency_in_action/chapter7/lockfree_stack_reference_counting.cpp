#include <atomic>
#include <memory>
template<typename T>
class lock_free_stack
{
private:
    struct node{
        std::shared_ptr<T> data_;
        std::shared_ptr<node> next_;
        node(T const& data) : data_(std::make_shared<T>(data)) {}
    };
    std::shared_ptr<node> head_;
public:
    void push(T const& data)
    {
        std::shared_ptr<node> const new_node = std::make_shared<node>(data);
        new_node->next_ = std::atomic_load(&head_);
        while(!std::atomic_compare_exchange_weak(&head_, &new_node->next_, new_node));
    }

    std::shared_ptr<T> pop()
    {
        std::shared_ptr<node> old_head = std::atomic_load(&head_);
        while(old_head && !std::atomic_compare_exchange_weak(&head_, &old_head, std::atomic_load(&old_head->next_)));
        if(old_head){
            std::atomic_store(&old_head->next_, std::shared_ptr<node>());
            return old_head->data_;
        }
        return std::shared_ptr<T>();
    }

    ~lock_free_stack()
    {
        while(pop());
    }
    
};