#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

// simple singly-linked node
struct Node{
    int value;
    Node* next;
    Node(int v) : value(v), next(nullptr){}
};

// atomic head pointer
std::atomic<Node*> head{nullptr};

// push a new node
void push(int v){
    Node* node = new Node(v);
    do{
        node->next = head.load(std::memory_order_acquire);
    }while(!head.compare_exchange_weak(node->next, node, std::memory_order_release, std::memory_order_relaxed));
}

// naive pop (ABA vulnerable)
Node* pop(){
    Node* old_head = head.load(std::memory_order_acquire);
    if(!old_head) return nullptr;
    Node* next = old_head->next;

    // if head == old_head(A), set head = next(B)
    if(head.compare_exchange_strong(old_head, next, std::memory_order_acq_rel, std::memory_order_acquire)){
        return old_head; // caller must delete
    }

    return nullptr; // CAS failure
}

int main()
{
    // synchronization primitives
    std::mutex mtx;
    std::condition_variable cv;
    bool t1_paused = false;
    bool t2_finished = false;

    // initial stack 3->2->1
    push(1);
    push(2);
    push(3);

    // thread 1: starts pop but pauses before CAS
    std::thread t1([&](){
        Node* old_head = head.load(std::memory_order_acquire);
        if(!old_head) return;
        Node* next = old_head->next;
        {
            std::unique_lock<std::mutex> lock(mtx);
            t1_paused = true;
            cv.notify_one();
            cv.wait(lock, [&]{return t2_finished;});
        }
        // resume an attemp CAS
        if(head.compare_exchange_strong(old_head, next, std::memory_order_acq_rel, std::memory_order_acquire)){
            std::cout << "T1 popped: " << old_head->value << "\n";
            delete old_head;
        }
        else{
            std::cout << "T1 CAS failed\n";
        }
    });

    // thread 2: performs pops and pushes to trigger ABA
    std::thread t2([&](){
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&](){return t1_paused;});
        }
        // pop all nodes
        Node* n1 = pop(); // 3
        Node* n2 = pop(); // 2
        Node* n3 = pop(); // 1
        if(n1) std::cout << "T2 popped: " << n1->value << "\n";
        if(n2) std::cout << "T2 popped: " << n2->value << "\n";
        if(n3) std::cout << "T2 popped: " << n3->value << "\n";
        // delete popped nodes to free memory
        delete n1;
        delete n2;
        delete n3;
        // push new nodes, potentially reusing memory
        push(6); // may reuse address of n1
        push(5);
        {
            std::unique_lock<std::mutex> lock(mtx);
            t2_finished = true;
            cv.notify_one();
        }
    });

    t1.join();
    t2.join();

    // clean up remaining nodes
    Node* node;
    while((node = pop()) != nullptr){
        delete node;
    }
    return 0;
}