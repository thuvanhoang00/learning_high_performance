#include "lockfreequeue_maybetrash.h"
#include "messagequeue.h"
#include "fifo1.h"
#include "fifo2.h"
#include "fifo3.h"
#include "fifo4.h"
#include "fifo5.h"
#include <benchmark/benchmark.h>

// #define SIZE 5000000

static void lockfree_test(benchmark::State& s)
{
    size_t SIZE = 1 << s.range(0);
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

// static void lockfree_test2(benchmark::State& s)
// {
//     for(auto state : s){
//         thu::LockFreeQueue<int> queue(SIZE);
//         auto push = [&](){for(int i=0; i<SIZE; i++) queue.tryPush(new int(i));};
//         auto pop = [&](){
//             for (int i = 0; i < SIZE; i++)
//             {
//                 // int* data;
//                 queue.tryPop();
//             };
//         };

//         std::thread t1(push);
//         std::thread t2(pop);
//         t1.join();
//         t2.join();

//     }
// }

static void msgqueue_test(benchmark::State& s)
{
    size_t SIZE = 1 << s.range(0);
    for(auto state : s){
        thu::MessageQueue<int> queue;
        Fifo4<int, std::allocator<int>> ff4(SIZE);
        ff4.capacity();
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

// static void fifo1_test(benchmark::State& s)
// {
//     for(auto state : s){
//         Fifo1<int, std::allocator<int>> ff1(SIZE);

//         auto push = [&](){for(int i=0; i<SIZE; i++) ff1.push(i);};
//         auto pop = [&](){
//             for (int i = 0; i < SIZE; i++)
//             {
//                 int data;
//                 ff1.pop(data);
//             };
//         };
//         std::thread t1(push);
//         std::thread t2(pop);
//         t1.join();
//         t2.join();
//     }
// }
static void fifo2_test(benchmark::State& s)
{
    size_t SIZE = 1 << s.range(0);
    for(auto state : s){
        Fifo2<int, std::allocator<int>> ff2(SIZE);

        auto push = [&](){for(int i=0; i<SIZE; i++) ff2.push(i);};
        auto pop = [&](){
            for (int i = 0; i < SIZE; i++)
            {
                int data;
                ff2.pop(data);
            };
        };
        std::thread t1(push);
        std::thread t2(pop);
        t1.join();
        t2.join();
    }
}

static void fifo3_test(benchmark::State& s)
{
    size_t SIZE = 1 << s.range(0);
    for(auto state : s){
        Fifo3<int, std::allocator<int>> ff3(SIZE);

        auto push = [&](){for(int i=0; i<SIZE; i++) ff3.push(i);};
        auto pop = [&](){
            for (int i = 0; i < SIZE; i++)
            {
                int data;
                ff3.pop(data);
            };
        };
        std::thread t1(push);
        std::thread t2(pop);
        t1.join();
        t2.join();
    }
}

static void fifo4_test(benchmark::State& s)
{
    size_t SIZE = 1 << s.range(0);
    for(auto state : s){
        Fifo4<int, std::allocator<int>> ff4(SIZE);

        auto push = [&](){for(int i=0; i<SIZE; i++) ff4.push(i);};
        auto pop = [&](){
            for (int i = 0; i < SIZE; i++)
            {
                int data;
                ff4.pop(data);
            };
        };
        std::thread t1(push);
        std::thread t2(pop);
        t1.join();
        t2.join();
    }
}

static void fifo5_test(benchmark::State& s)
{
    size_t SIZE = 1 << s.range(0);
    for(auto state : s){
        Fifo5<int, std::allocator<int>> ff5(SIZE);

        auto push = [&](){for(int i=0; i<SIZE; i++) ff5.push(i);};
        auto pop = [&](){
            for (int i = 0; i < SIZE; i++)
            {
                int data;
                ff5.pop(data);
            };
        };
        std::thread t1(push);
        std::thread t2(pop);
        t1.join();
        t2.join();
    }
}

BENCHMARK(msgqueue_test)->DenseRange(25, 30);
BENCHMARK(fifo2_test)->DenseRange(25, 30);
BENCHMARK(fifo3_test)->DenseRange(25, 30);
BENCHMARK(fifo4_test)->DenseRange(25, 30);
BENCHMARK(fifo5_test)->DenseRange(25, 30);
BENCHMARK_MAIN();
