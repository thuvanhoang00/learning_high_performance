#pragma once
#include <atomic>
#include <thread>
#include <iostream>
template<typename T>
class LockFreeQueue_Without_Dummy_Node{
private:
    struct Node{
        T data_;
        Node* next_;
        Node(const T& data) : data_(data), next_(nullptr) {}
        // Node() : data_{}, next_(nullptr) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

public:
    LockFreeQueue_Without_Dummy_Node() : head_(nullptr), tail_(nullptr) {}
    ~LockFreeQueue_Without_Dummy_Node(){
        while(head_){
            auto old_head = head_.load();
            head_.store(old_head->next_);
            delete old_head;
        }
    }

    bool push(const T& value){
        Node* new_node = new Node(value);
        auto old_tail = tail_.load(std::memory_order_acquire); // (1) 
        if(old_tail){
            old_tail->next_ = new_node;
        }
        else{
            head_.store(new_node, std::memory_order_release); // (2) syncrho-with (4)
        }

        tail_.store(new_node, std::memory_order_release); // (3)
        return true;

    }

    bool pop(T& value){
        Node* const old_head = head_.load(std::memory_order_acquire); // (4)
        if(!old_head){
            return false;
        }

        Node* next = old_head->next_;
        if(!next){
            tail_.store(nullptr, std::memory_order_release); // (5) synchronization-with (1)
        }

        head_.store(next, std::memory_order_release); // (6)
        value = old_head->data_;
        delete old_head;
        return true;
    }
};

// #define N 100000

// int main(){
//     LockFreeQueue<int> lfq;

//     std::thread t1([&](){
//         for(int i=0; i<N; ++i){
//             lfq.push(i);
//          }
//     });

//     std::thread t2([&](){
//         for(int i=0; i<N; ++i){
//             int data;
//             lfq.pop(data);
//             std::cout << "popped: " << data << std::endl;
//         }
//     });

//     t1.join();
//     t2.join();

//     return 0;
// }