#include <atomic>
#include <memory>
#include "hazard_pointer.h"
template<typename T>
class lock_free_stack
{
private:
    // struct node{
    //     T data_;
    //     node* next_;
    //     node(T const& data) : data_(data) {}
    // };
    struct node{
        std::shared_ptr<T> data_;
        node* next_;
        node(T const& data) : data_(std::make_shared<T>(data)) {}
    };
    std::atomic<node*> head_;

    std::atomic<unsigned> threads_in_pop_;
    void try_reclaim(node* old_head)
    {
        if(threads_in_pop_ == 1){
            node* nodes_to_delete = to_be_deleted_.exchange(nullptr);
            if(!--threads_in_pop_){
                delete_nodes(nodes_to_delete);
            }
            else if(nodes_to_delete){
                chain_pending_nodes(nodes_to_delete);
            }
            delete old_head;
        }
        else{
            chain_pending_node(old_head);
            --threads_in_pop_;
        }
    }

    void chain_pending_nodes(node* nodes)
    {
        node* last = nodes;
        while(node* const next = last->next_)
        {
            last = next;
        }
        chain_pending_nodes(nodes, last);
    }
    
    void chain_pending_nodes(node* first, node* last)
    {
        last->next_ = to_be_deleted_;
        while(!to_be_deleted_.compare_exchange_weak(last->next_, first));
    }

    void chain_pending_node(node* n)
    {
        chain_pending_nodes(n,n);
    }

    std::atomic<node*> to_be_deleted_;
    static void delete_nodes(node* nodes)
    {
        while(nodes){
            node* next = nodes->next_;
            delete nodes;
            nodes = next;
        }
    }
public:
    void push(T const& data)
    {
        node* const new_node = new node(data);
        new_node->next_ = head_.load();
        while(!head_.compare_exchange_weak(new_node->next_, new_node));
    }

    std::shared_ptr<T> pop()
    {
        std::atomic<void*>& hp = get_hazard_pointer_for_current_thread();
        node* old_head = head_.load();
        do
        {
            node* temp;
            do{
                temp = old_head;
                hp.store(old_head);
                old_head = head_.load();
            }
            while(old_head != temp);
        }
        while(old_head && !head_.compare_exchange_strong(old_head, old_head->next_));

        hp.store(nullptr);
        std::shared_ptr<T> res;
        if(old_head){
            res.swap(old_head->data_);
            if(outstanding_hazard_pointers_for(old_head)){
                reclaim_later(old_head);
            }
            else{
                delete old_head;
            }
            delete_nodes_with_no_hazards();
        }
        return res;
    }
    
};