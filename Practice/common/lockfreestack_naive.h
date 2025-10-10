#include <atomic>
#include <thread>
#include <iostream>

template<typename T>
class LockFreeStack{
private:
    struct Node{
        Node(cosnt T& data) : data_(data), next_(nullptr) {}
        T data_;
        Node* next_;
    };
    std::atomic<Node*> head_;
public:
    LockFreeStack() : head_(nullptr) {}

    bool push(const T& value){
        Node* new_node = new Node(value);
        
        new_node->next_ = head_;

        // Checking whether head_ changed or not
        // if changed then point new_node_next to new head
        // otherwise assign head to new_node
        while(!head_.compare_exchange_weak(new_node->next_, new_node));
        
        return true;
    }

    bool pop(T& value){
        Node* popped_node = head_;
        if(!popped_node){
            return false;
        }

        // Node* next_node = head_->next_;
        // checking head_
        while(!head_.compare_exchange_weak(popped_node, popped_node->next_));

        value = popped_node->data_;
        return true;
    }
};

#define N 1000000

int main(){
    LockFreeStack<int> lfs;

    std::thread t1([&](){
        for(int i=0; i<N; ++i){
            lfs.push(i);
            // std::cout << "pushed: " << i << std::endl;
        }
    });

    std::thread t2([&](){
        for(int i=0; i<N; ++i){
            int data;
            lfs.pop(data);
            std::cout << "popped: " << data << std::endl;
        }
    });

    t1.join();
    t2.join();
    return 0;
}