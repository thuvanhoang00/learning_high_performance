#include "lockfreequeue.h"
#include "messagequeue.h"
#include <benchmark/benchmark.h>
#define SIZE 5000000

static void lockfree_test(benchmark::State& s)
{
    for(auto state : s){
        thu::LockFreeQueue<int> queue(SIZE);
        auto push = [&](){for(int i=0; i<SIZE; i++) queue.push(i);};
        auto pop = [&](){
            for (int i = 0; i < SIZE; i++)
            {
                int data;
                queue.pop(data);
            };
        };

        std::thread t1(push);
        std::thread t2(pop);
        t1.join();
        t2.join();

    }
}

static void msgqueue_test(benchmark::State& s)
{
    for(auto state : s){
        thu::MessageQueue<int> queue;
        thu::LockFreeQueue<int> __queue(SIZE);

        auto push = [&](){for(int i=0; i<SIZE; i++) queue.push(i);};
        auto pop = [&](){
            for (int i = 0; i < SIZE; i++)
            {
                int data;
                queue.pop(data);
            };
        };
        std::thread t1(push);
        std::thread t2(pop);
        t1.join();
        t2.join();
    }
}

BENCHMARK(lockfree_test);
BENCHMARK(msgqueue_test);

BENCHMARK_MAIN();
