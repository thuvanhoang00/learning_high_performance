#include "lockfreequeue.h"
#include "lockfreequeue_without_dummy_node.h"
#include "bench.h"
#include "fifo2.h"
#include "fifo3.h"

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

void bench_fifo2(){
    Fifo2<int> lfq(N);

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

void bench_fifo3(){
    Fifo3<int> lfq(N);

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
    time_it("FIFO2:", bench_fifo2);
    time_it("FIFO3:", bench_fifo3);

    return 0;
}