#include <memory>
#include <atomic>

template<typename T>
class lock_free_queue
{
private:
    struct node;
    
    struct counted_node_ptr{
        int external_count_;
        node* ptr_;
    };

    struct node_counter
    {
        unsigned internal_count:30;
        unsigned external_counters:2;
    };
    

    struct node{
        // std::shared_ptr<T> data_;
        // node* next_;
        // node() : next_(nullptr) {}
        std::atomic<T*> data_;
        std::atomic<node_counter> count_;
        std::atomic<counted_node_ptr*> next_;
        node(){
            node_counter new_count;
            new_count.internal_count = 0;
            new_count.external_counters = 2;
            count_.store(new_count);

            next_.ptr_ = nullptr;
            next_.external_count_ = 0;
        }

        void release_ref(){
            node_counter old_counter = count_.load(std::memory_order_relaxed);
            node_counter new_counter;
            do{
                new_counter = old_counter;
                --new_counter.internal_count;
            }
            while(!count_.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));

            if(!new_counter.internal_count && !new_counter.external_counters){
                delete this;
            }
        }
    };

    static void increase_external_count(std::atomic<counted_node_ptr>& counter, counted_node_ptr& old_counter){
        counted_node_ptr new_counter;
        do{
            new_counter = old_counter;
            ++new_counter.external_count_;
        }
        while(!counter.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));
        old_counter.external_count_ = new_counter.external_count_;
    }

    static void free_external_counter(counted_node_ptr& old_node_ptr){
        node* const ptr = old_node_ptr.ptr_;
        int const count_increase = old_node_ptr.external_count_ - 2;
        node_counter old_counter = ptr->count_.load(std::memory_order_relaxed);
        node_counter new_counter;
        do{
            new_counter = old_counter;
            --new_counter.external_counters;
            new_counter.internal_count += count_increase;
        }
        while(!ptr->count_.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));

        if(!new_counter.internal_count && !new_counter.external_counters){
            delete ptr;
        }
    }

    // std::atomic<node*> head_;
    // std::atomic<node*> tail_;

    std::atomic<counted_node_ptr> head_;
    std::atomic<counted_node_ptr> tail_;

    node* pop_head()
    {
        node* const old_head = head_.load();
        if(old_head == tail_.load()){
            return nullptr;
        }
        head_.store(old_head->next_);
        return old_head;
    }

    void set_new_tail(counted_node_ptr& old_tail, counted_node_ptr const& new_tail){
        node* const current_tail_ptr = old_tail.ptr_;
        while (!tail_.compare_exchange_weak(old_tail, new_tail) && old_tail.ptr_ == current_tail_ptr);

        if(old_tail.ptr_ == current_tail_ptr){
            free_external_counter(old_tail);
        }
        else{
            current_tail_ptr->release_ref();
        }        
    }

public:
    lock_free_queue() : head_(new counted_node_ptr), tail_(head_.load()) {}
    lock_free_queue(const lock_free_queue&) = delete;
    lock_free_queue& operator=(const lock_free_queue&) = delete;
    ~lock_free_queue(){
        while(counted_node_ptr* const old_head = head_.load()){
            head_.store(old_head->next_);
            delete old_head;
        }
    }
    std::shared_ptr<T> pop()
    {
        // node* old_head = pop_head();
        // if(!old_head){
        //     return std::shared_ptr<T>();
        // }
        // std::shared_ptr<T> const res(old_head->data_);
        // delete old_head;
        // return res;
        counted_node_ptr* old_head = head_.load(std::memory_order_relaxed);
        for(;;)
        {
            increase_external_count(head_, old_head);
            node* const ptr = old_head.ptr_;
            if(ptr == tail_.load().ptr_){
                ptr->release_ref();
                return std::unique_ptr<T>();
            }
            counted_node_ptr* next = ptr->next_.load();
            if(head_.compare_exchange_strong(old_head, next)){
                T* const res = ptr->data_.exchange(nullptr);
                free_external_counter(old_head);
                return std::unique_ptr<T>(res);
            }
            ptr->release_ref();
        }
    }

    // Multi threads in push
    void push(T new_value){
        // std::unique_ptr<T> new_data(new T(new_value));
        // counted_node_ptr new_next;
        // new_next.ptr_ = new node;
        // new_next.external_count_ = 1;
        // for(;;){
        //     node* const old_tail = tail_.load();
        //     T* old_data = nullptr;
        //     if(old_tail->data_.compare_exchange_strong(old_data, new_data.get())){
        //         old_tail->next_ = new_next;
        //         tail_.store(new_next.ptr_);
        //         new_data.release();
        //         break;
        //     }
        // }
        std::unique_ptr<T> new_data(new T(new_value));
        counted_node_ptr new_next;
        new_next.ptr_ = new node;
        new_next.external_count_ = 1;
        counted_node_ptr old_tail = tail_.load();
        for(;;){
            increase_external_count(tail_, old_tail);
            T* old_data = nullptr;
            if(old_tail.ptr_->data_.compare_exchange_strong(old_data, new_data.get())){
                // old_tail.ptr_->next_ = new_next;
                // old_tail = tail_.exchange(new_next);
                // free_external_counter(old_tail);
                // new_data.release();
                // break;
                counted_node_ptr old_next = {};
                if(!old_tail.ptr_->next_.compare_exchange_strong(old_next, new_next)){
                    delete new_next.ptr_;
                    new_next = old_next;
                }
                set_new_tail(old_tail, new_next);
                new_data.release();
                break;
            }
            else{
                counted_node_ptr old_next = {0};
                if(old_tail.ptr_->next_.compare_exchange_strong(old_next, new_next)){
                    old_next = new_next;
                    new_next.ptr_ = new node;
                }
                set_new_tail(old_tail, old_next);
            }
        }
    }
};
