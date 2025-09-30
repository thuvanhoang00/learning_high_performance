#include "bench.h"
#include <atomic>
#include <thread>
#include <cassert>

std::atomic<bool> x=false;
std::atomic<bool> y=false;
std::atomic<int> z=0;

void store_x(){
    x.store(true, std::memory_order_seq_cst);
}

void store_y(){
    y.store(true, std::memory_order_seq_cst);
}

void read_x_then_y(){
    while(!x.load(std::memory_order_seq_cst));
    if(y.load(std::memory_order_seq_cst)){
        z++;
    }
}

void read_y_then_x(){
    while(!y.load(std::memory_order_seq_cst));
    if(x.load(std::memory_order_seq_cst)){
        z++;
    }
}


int main(){
    auto t1 = std::thread(store_x);
    auto t2 = std::thread(store_y);
    auto t3 = std::thread(read_x_then_y);
    auto t4 = std::thread(read_y_then_x);
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    assert(z != 0);    
    
    return 0;
}