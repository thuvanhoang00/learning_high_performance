#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <iostream>

constexpr int HP_MAX = 2; // hazard pointers per thread
constexpr int THREADS = 4; // Max threads
constexpr int RETIRE_THRESHOLD = 10;

// Hazard pointer structure
struct HazardPointer{
    std::atomic<void*> pointer;
    std::atomic_flag active = ATOMIC_FLAG_INIT;
};

HazardPointer hp_list[THREADS*HP_MAX]; // Global HP array

// Thread-local variables
thread_local int thread_id = -1;
thread_local std::vector<void*> retired_list;

// Assign unique thread IDs
std::atomic<int> global_id{0};

class LockFreeStack{
private:
    struct Node{
        Node* next;
        int data;
        Node(int val) : next(nullptr), data(val){}
    };

    std::atomic<Node*> head{nullptr};

public:
    void push(int value){
        Node* new_node = new Node(value);
        new_node->next = head.load();
        while(!head.compare_exchange_weak(new_node->next, new_node));
    }

    bool pop(int& value){
        HazardPointer* hp = acquire_hp();
        if(!hp) return false;

        Node* old_head;
        do{
            old_head = head.load();
            hp->pointer.store(old_head);
        } while(old_head != head.load());

        if(!old_head){
            release_hp(hp);
            return false;
        }

        Node* next = old_head->next;
        if(head.compare_exchange_strong(old_head, next)){
            value = old_head->data;
            release_hp(hp);
            retire_node(old_head);
            return true;
        }

        release_hp(hp);
        return false;
    }

private:
    HazardPointer* acquire_hp(){
        for(int i=0; i<HP_MAX; i++){
            int index = thread_id * HP_MAX + i;
            if(!hp_list[index].active.test_and_set()){
                return &hp_list[index];
            }
        }
        return nullptr; // No available HP slot
    }

    void release_hp(HazardPointer* hp){
        hp->pointer.store(nullptr);
        hp->active.clear();
    }

    void retire_node(Node* node){
        retired_list.push_back(node);
        if(retired_list.size() >= RETIRE_THRESHOLD){
            scan();
        }
    }

    void scan(){
        // Collect all protected pointers
        std::vector<void*> protected_ptrs;
        for(int i=0; i<THREADS*HP_MAX; i++){
            void* p = hp_list[i].pointer.load();
            if(p) protected_ptrs.push_back(p);
        }

        // Partition retired list
        std::vector<void*> to_delete;
        auto it = retired_list.begin();
        while(it != retired_list.end()){
            void* p = *it;
            bool found = false;

            for(void* protected_ptr : protected_ptrs){
                if(p==protected_ptr){
                    found = true;
                    break;
                }
            }

            if(!found){
                to_delete.push_back(p);
                it = retired_list.erase(it);
            }
            else{
                ++it;
            }

            // Delete safe nodes
            for(void* p : to_delete){
                delete static_cast<Node*>(p);
            }
        }
    }
};

// Test function
void test_stack(LockFreeStack& stack){
    thread_id = global_id.fetch_add(1);

    for(int i=0; i<1000; i++){
        stack.push(i);
        int val;
        if(stack.pop(val)){
            // Do sth with val
            std::cout << "val: " << val << std::endl;
        }
    }
}

int main()
{
    LockFreeStack stack;
    std::vector<std::thread> threads;
    for(int i=0; i<THREADS; i++){
        threads.emplace_back(test_stack, std::ref(stack));
    }

    for(auto& t : threads){
        t.join();
    }
    return 0;
}