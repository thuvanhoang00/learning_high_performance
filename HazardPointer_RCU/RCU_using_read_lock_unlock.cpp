#include <atomic>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>
constexpr int num_readers = 4;

std::atomic<int*> shared_data;
std::atomic<int> writer_generation{0};
std::vector<std::atomic<int>> reader_generations(num_readers);

void rcu_read_lock(){
    // In this simple implementation, do nothing
}

void rcu_read_unlock(int index){
    reader_generations[index].store(writer_generation.load());
}

void reader(int index){
    while(true){
        rcu_read_lock();
        int* data = shared_data.load();
        if(data){
            int value = *data;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Reader " << index << " read value: " << value << std::endl;
        }
        rcu_read_unlock(index);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void synchronize_rcu(int current_generation){
    while(true){
        bool all_updated = true;
        for(const auto& rg : reader_generations){
            if(rg.load() < current_generation){
                all_updated = false;
                break;
            }
        }
        if(all_updated){
            break;
        }
        std::this_thread::yield();
    }
}

void writer(){
    int new_value = 1;
    while(true){
        int* new_data = new int(new_value);
        int current_generation = writer_generation.fetch_add(1) + 1;
        int* old_data = shared_data.exchange(new_data);
        std::cout << "Writer updated to " << new_value << std::endl;
        synchronize_rcu(current_generation);
        delete old_data;
        new_value++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(){
    shared_data = new int(0);
    // reader_generations.resize(num_readers);
    // for(int i=0; i<num_readers; i++){
    //     reader_generations.emplace_back(0);
    // }
    for(auto& rg : reader_generations){
        rg.store(0);
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

    delete shared_data.load();
    return 0;
}