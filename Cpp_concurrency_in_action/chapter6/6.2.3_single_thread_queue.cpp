#include <memory>
template<typename T>
class queue
{
private:
    struct node{
        T data_;
        std::unique_ptr<node> next_;
        node(T data) : data_(data) {}
    };
    std::unique_ptr<node> head_;
    node* tail_;
public:
    queue() : tail_(nullptr) {}
    queue(const queue& ) = delete;
    queue& operator=(const queue&) = delete;
    std::shared_ptr<T> try_pop(){
        if(!head_){
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(head_->data())));
        std::unique_ptr<node> const old_head = std::move(head_);
        head_ = std::move(old_head->next_);
        if(!head_){
            tail_ = nullptr;
        }
        return res;
    }
    void push(T new_value){
        std::unique_ptr<node> p(new node(std::move(new_value)));
        node* const new_tail = p.get();
        if(tail_){
            tail_->next_ = std::move(p);
        }
        else{
            head_ = std::move(p);
        }
        tail_ = new_tail;
    }
};