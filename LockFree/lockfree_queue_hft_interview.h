#ifndef LOCKFREE_QUEUE_HFT
#define LOCKFREE_QUEUE_HFT
#include <atomic>
#include <memory>
#include <optional>

template<typename T>
class LockFreeQueue
{
private:
    // linked-list of data
    struct Node{
        Node(const T& data, std::shared_ptr<Node> next) : _data(data){
            _next.store(next);
        }

        Node(const T& data) : _data(data){
            _next.store(nullptr);
        }

        T _data;
        std::atomic<std::shared_ptr<Node>> _next;
    };
    std::atomic<std::shared_ptr<Node>> _head;
    std::atomic<std::shared_ptr<Node>> _tail;

public:
    LockFreeQueue(){
        _head.store(nullptr); // "store" is the correct way to update value of atomic variable
        _tail.store(nullptr);
    }

    void push(const T& data){
        std::shared_ptr<Node> new_node = std::make_shared<Node>(data);
        std::shared_ptr<Node> old_tail = _tail.load();
        
        while(!_tail.compare_exchange_weak(old_tail, new_node)){
            old_tail = _tail.load();
        }

        if(old_tail != nullptr){
            old_tail->_next.store(new_node);
        }
        else{
            _head.store(new_node);
        }
    }

    std::optional<T> pop(){
        std::shared_ptr<Node> old_head = _head.load();
        while(old_head != nullptr && !_head.compare_exchange_weak(old_head, old_head->_next.load())){
            old_head = _head.load();
        }

        if(old_head == nullptr){
            return std::nullopt;
        }

        return old_head->_data;
    }
};


#endif