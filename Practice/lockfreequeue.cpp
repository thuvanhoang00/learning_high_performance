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
            head_ = new_node;
            tail_ = new_node;
        }
        else{
            Node* old = tail_.load();
            do{
                // append to tail
                (*tail_).next_ = new_node;
            }
            while(!tail_.compare_exchange_weak(old, new_node)); // checking tail
        }
        return true;

    }

    bool pop(T& value){
        Node* popped_node = head_.load();

        if(!popped_node){
            return false;
        }

        // do{
        //     popped_node->next_ = (*head_).next_;
        // }
        // checking head_
        while(!head_.compare_exchange_weak(popped_node, popped_node->next_));
    
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