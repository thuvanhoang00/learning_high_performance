#include <iostream>
#include <thread>
#include "lockfree_queue_multithread_in_push_using_reference_counting.h"

int main(){
    lock_free_queue<int> queue;

    std::thread t1([&](){
        for(int i=0; i<1'000'000; ++i){
            queue.push(i);
        }
    });

    std::thread t2([&](){
        while (true)
        {
            auto ptr = queue.pop();
            if(!ptr) std::cout << "popped: " << *ptr << " ";
        }
        
    });

    return 0;
}