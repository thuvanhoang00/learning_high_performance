#include <utility>
#include <list>
#include <mutex>
#include <vector>
#include <memory>
#include <shared_mutex>
#include <map>

/*
* No chance of deadlock because there is only one way order of acquiring lock -> no posibility of different lock orders in different threads
*/
template<typename T>
class threadsafe_list
{
private:
    struct node{
        std::mutex m_;
        std::shared_ptr<T> data_;
        std::unique_ptr<node> next_;
        node() : next_() {}
        node(T const& value) : data_(std::make_shared<T>(value)) {}
    };

    node head_;

public:
    threadsafe_list() {}
    ~threadsafe_list() {remove_if([](node const&){return true;});}
    threadsafe_list(threadsafe_list const& ) = delete;
    threadsafe_list& operator=(threadsafe_list const&) = delete;
    
    void push_front(T const& value)
    {
        std::unique_ptr<node> new_node(new node(value));
        std::lock_guard<std::mutex> lk(head_.m_);
        new_node->next_ = std::move(head_.next_);
        head_.next_ = std::move(new_node);
    }

    template<typename Function>
    void for_each(Function f)
    {
        node* current = &head_;
        std::unique_lock<std::mutex> lk(head_.m_);
        while(node* const next = current->next_.get()){
            std::unique_lock<std::mutex> next_lk(next->m_);
            lk.unlock();
            f(*next->data_);
            current = next;
            lk = std::move(next_lk);
        }
    }

    template<typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate p)
    {
        node* current = &head_;
        std::unique_lock<std::mutex> lk(head_.m_);
        while (node* const next = current->next_.get()){
            std::unique_lock<std::mutex> next_lk(next->m_);
            lk.unlock();
            if(p(*next->data_)){
                return next->data_;
            }
            current = next;
            lk = std::move(next_lk);
        }
        return std::shared_ptr<T>();
    }

    template<typename Predicate>
    void remove_if(Predicate p)
    {
        node* current = &head_;
        std::unique_lock<std::mutex> lk(head_.m_);
        while(node* const next = current->next_.get()){
            std::unique_lock<std::mutex> next_lk(next->m_);
            if(p(*next->data_)){
                std::unique_ptr<node> old_next = std::move(current->next_);
                current->next_ = std::move(next->next_);
                next_lk.unlock();
            }
            else{
                lk.unlock();
                current = next;
                lk = std::move(next_lk);
            }
        }
    }
}; 