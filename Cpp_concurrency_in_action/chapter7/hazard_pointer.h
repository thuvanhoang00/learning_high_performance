#include <atomic>
#include <thread>
#include <functional>

unsigned const max_hazard_pointers = 100;
struct hazard_pointer
{
    std::atomic<std::thread::id> id_;
    std::atomic<void*> pointer_;
};
hazard_pointer hazard_pointers[max_hazard_pointers];

class hp_owner
{
    hazard_pointer* hp_;
public:
    hp_owner(hp_owner const&) = delete;
    hp_owner& operator=(hp_owner const&) = delete;
    hp_owner() : hp_(nullptr)
    {
        for(unsigned i = 0; i<max_hazard_pointers; ++i)
        {
            std::thread::id old_id;
            if(hazard_pointers[i].id_.compare_exchange_strong(old_id, std::this_thread::get_id())){
                hp_ = &hazard_pointers[i];
                break;
            }
        }
        if(!hp_)
        {
            throw std::runtime_error("No hazard pointers available");
        }
    }
    
    std::atomic<void*>& get_pointer()
    {
        return hp_->pointer_;
    }

    ~hp_owner()
    {
        hp_->pointer_.store(nullptr);
        hp_->id_.store(std::thread::id());
    }
};

std::atomic<void*>& get_hazard_pointer_for_current_thread()
{
    thread_local static hp_owner hazard;
    return hazard.get_pointer();
}

bool out_standing_hazard_pointers_for(void* p)
{
    for(unsigned i=0; i<max_hazard_pointers;++i){
        if(hazard_pointers[i].pointer_.load() == p)
        {
            return true;
        }
    }
    return false;
}

template<typename T>
void do_delete(void* p)
{
    delete static_cast<T*>(p);
}

struct data_to_reclaim
{
    void* data_;
    std::function<void(void*)> deleter_;
    data_to_reclaim *next_;
    
    template<typename T>
    data_to_reclaim(T* p) : data_(p), deleter_(&do_delete<T>), next_(0) {}

    ~data_to_reclaim(){ deleter_(data_);}
};

std::atomic<data_to_reclaim*> nodes_to_reclaim;
void add_to_reclaim_list(data_to_reclaim* node)
{
    node->next_ = nodes_to_reclaim.load();
    while(!nodes_to_reclaim.compare_exchange_weak(node->next_, node));
}

template<typename T>
void reclaim_later(T* data)
{
    add_to_reclaim_list(new data_to_reclaim(data));
}

void delete_nodes_with_no_hazards()
{
    data_to_reclaim* current = nodes_to_reclaim.exchange(nullptr);
    while(current)
    {
        data_to_reclaim* const next = current->next_;
        if(!out_standing_hazard_pointers_for(current->data_)){
            delete current;
        }
        else{
            add_to_reclaim_list(current);
        }
        current = next;
    }
}
