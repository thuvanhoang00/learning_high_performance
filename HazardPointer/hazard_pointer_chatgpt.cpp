#include <atomic>
#include <memory>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>

// Node structure
template<typename T>
struct Node{
    T data;
    Node* next;
    explicit Node(const T& d) : data(d), next(nullptr){}
};

// Global hazard pointer slots
static const unsigned MAX_HAZARD_POINTERS = 100;
std::atomic<std::uintptr_t> hazard_pointers[MAX_HAZARD_POINTERS];

// RAII wapper for a single hazard pointer
class HazardPointer{
private:
    std::atomic<std::uintptr_t>* hp;
public:
    HazardPointer() : hp(nullptr){
        for(unsigned int i = 0; i < MAX_HAZARD_POINTERS; i++){
            std::uintptr_t expected = 0;
            if(hazard_pointers[i].compare_exchange_strong(expected, 1)){
                hp = &hazard_pointers[i];
                break;
            }
        }
    }

    void set(std::uintptr_t ptr) {hp->store(ptr);}
    void clear(){hp->store(0);}
    ~HazardPointer(){if(hp) hp->store(0);}
};

// Lock-free stack with hazard-pointer-based reclamation
template<typename T>
class LockFreeStack{
private:
    std::atomic<Node<T>*> head{nullptr};
    static std::atomic<Node<T>*> to_be_deleted;
public:
    void push(const T& x){
        Node<T>* n = new Node<T>(x);
        n->next = head.load();
        while(!head.compare_exchange_weak(n->next,n));
    }

    std::shared_ptr<T> pop(){
        HazardPointer hz;
        Node<T>* old_head = head.load();
        do{
            Node<T>* temp;
            do{
                temp = old_head;
                hz.set(reinterpret_cast<std::uintptr_t>(old_head));
                old_head = head.load();
            } while(old_head != temp);
        } while(old_head && !head.compare_exchange_strong(old_head, old_head->next));
        hz.clear();

        if(!old_head) return{};
        
        std::shared_ptr<T> res;
        res.reset(&old_head->data, [](T*){});
        retire(old_head);
        reclaim();
        return res;
    }

private:
    static void retire(Node<T>* node){
        node->next = to_be_deleted.load();
        while(!to_be_deleted.compare_exchange_weak(node->next, node));
    }

    static void reclaim(){
        Node<T>* list = to_be_deleted.exchange(nullptr);
        while(list){
            Node<T>* next = list->next;
            bool hazard = false;
            for(unsigned int i=0; i<MAX_HAZARD_POINTERS; i++){
                if(hazard_pointers[i].load() == reinterpret_cast<std::uintptr_t>(list)){
                    hazard = true;
                    break;
                }
            }

            if(!hazard){
                delete list;
            } else{
                retire(list);
            }
            list = next;
        }
    }
};

template<typename T>
std::atomic<Node<T>*> LockFreeStack<T>::to_be_deleted{nullptr};

int main(){
    LockFreeStack<int> stack;

    // Push initial values onto the stack
    for(int i=0; i<10; i++){
        stack.push(i);
    }

    // Lambda function for popping elements from the stack
    auto pop_task = [&stack](int thread_id){
        for(int i=0; i<5; i++){
            auto value = stack.pop();
            if(value){
                std::cout << "Thread " << thread_id << " popped: " << *value << "\n";
            }
            else{
                std::cout << "Thread " << thread_id << " found stack empty\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };

    // Launch multiple threads to pop elements concurrently
    std::vector<std::thread> threads;
    for(int i=0; i<3; i++){
        threads.emplace_back(pop_task, i);
    }

    for(auto& t : threads){
        t.join();
    }

    std::cout << "All threads have completed their operations\n";

    return 0;
}