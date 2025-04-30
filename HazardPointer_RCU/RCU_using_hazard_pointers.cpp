#include <atomic>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>

std::atomic<int*> shared_data;
std::vector<std::atomic<int*>*> all_hazard_ptrs;

void reader(int index){
    std::atomic<int*>& hazard_ptr = *all_hazard_ptrs[index];
    while(true){
        int* data = shared_data.load();
        hazard_ptr.store(data);
        if(data){
            int value = *data;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Reader " << index << " read value: " << value << std::endl;
        }
        hazard_ptr.store(nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void writer(){
    int new_value = 1;
    while(true){
        int* new_data = new int(new_value);
        int* old_data = shared_data.exchange(new_data);
        std::cout << "Writer updated to " << new_value << std::endl;
        // Wait until no hazard_ptr points to old_data
        while(true){
            bool still_in_use = true;
            for(auto hp : all_hazard_ptrs){
                if(hp->load() == old_data){
                    still_in_use = true;
                    break;
                }
            }
            if(!still_in_use){
                delete old_data;
                break;
            }
            std::this_thread::yield();
        }
        new_value++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(){
    const int num_readers = 4;
    shared_data = new int(0);
    // create hazard pointers
    for(int i=0; i<num_readers; i++){
        all_hazard_ptrs.push_back(new std::atomic<int*>(nullptr));
    }
    // start reader threads
    std::vector<std::thread> readers;
    for(int i=0; i<num_readers; i++){
        readers.emplace_back(reader, i);
    }

    // start writer thread
    std::thread writer_thread(writer);
    // let them run for a while
    std::this_thread::sleep_for(std::chrono::seconds(10));
    // cleanup 
    for(auto& t : readers){
        t.detach();
    }
    writer_thread.detach();
    for(auto hp : all_hazard_ptrs){
        delete hp;
    }
    delete shared_data.load();
    return 0;
}