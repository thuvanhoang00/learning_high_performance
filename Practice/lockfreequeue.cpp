#include <atomic>
#include <thread>
#include <iostream>
template<typename T>
class LockFreeQueue{
private:
    struct Node{
        T data_;
        Node* next_;
        Node(const T& data) : data_(data), next_(nullptr) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

public:
    LockFreeQueue() : head_(nullptr), tail_(nullptr) {}

    bool push(const T& value){
        Node* new_node = new Node(value);
        if(!tail_.load()){
            head_.store(new_node);
        }
        new_node->next_ = tail_;
        tail_.store(new_node);
        return true;

    }

    bool pop(T& value){
        if(!head_.load()){
            return false;
        }
        auto popped_node = head_.load();
        head_.store(popped_node->next_);
        value = popped_node->data_;
        return true;
    }
};

#define N 1000000

int main(){
    LockFreeQueue<int> lfq;

    std::thread t1([&](){
        for(int i=0; i<N; ++i){
            lfq.push(i);
            // std::cout << "pushed: " << i << std::endl;
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