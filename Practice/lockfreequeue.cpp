#include <atomic>
#include <thread>
#include <iostream>
template<typename T>
class LockFreeQueue{
private:
    struct Node{
        T data_;
        Node* next_;
        // Node(const T& data) : data_(data), next_(nullptr) {}
        Node() : data_{}, next_(nullptr) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

public:
    LockFreeQueue() : head_(new Node), tail_(head_.load()) {}
    ~LockFreeQueue(){
        while(head_){
            auto old_head = head_.load();
            head_.store(old_head->next_);
            delete old_head;
        }
    }

    bool push(const T& value){
        Node* new_node = new Node();
        auto old_tail = tail_.load();
        old_tail->next_ = new_node;
        old_tail->data_ = value;
        tail_.store(new_node, std::memory_order_release);
        return true;

    }

    bool pop(T& value){

#ifdef NAIVE
        if(head_.load() == tail_.load(std::memory_order_acquire)){
            return false;
        }
        auto popped_node = head_.load();
        head_.store(popped_node->next_);
        value = popped_node->data_;
        return true;
#endif

        Node* const old_head = head_.load();
        if(old_head == tail_.load(std::memory_order_acquire)){
            return false;
        }
        head_.store(old_head->next_);
        value = old_head->data_;
        delete old_head;
        return true;
    }
};

#define N 100000

int main(){
    LockFreeQueue<int> lfq;

    std::thread t1([&](){
        for(int i=0; i<N; ++i){
            lfq.push(i);
         }
    });

    std::thread t2([&](){
        for(int i=0; i<N; ++i){
            int data;
            lfq.pop(data);
            std::cout << "popped: " << data << std::endl;
        }
    });

    t1.join();
    t2.join();

    return 0;
}