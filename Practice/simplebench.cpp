#include "lockfreequeue.h"
#include "lockfreequeue_without_dummy_node.h"
#include "bench.h"
#define N 10000000

void bench_lock_free_queue(){
    LockFreeQueue<int> lfq;

    std::thread t1([&](){
        for(int i=0; i<N; ++i){
            lfq.push(i);
         }
    });

    std::thread t2([&](){
        for(int i=0; i<N; ++i){
            int data;
            lfq.pop(data);
            // std::cout << "popped: " << data << std::endl;
        }
    });

    t1.join();
    t2.join();
}

int main(){

    time_it("LockFreeQueue", bench_lock_free_queue);
    return 0;
}