#include "fifo2.h"
#include "fifo3.h"
#include <benchmark/benchmark.h>
#include <thread>

// void bench_fifo2(benchmark::State& s){
//     int N = s.range();

//     Fifo2<int> ff(N);
//     std::jthread t1([&](){
//         for(int i=0; i<N; ++i){
//             ff.push(i);
//         }
//     });
//     std::jthread t2([&](){
//         for(int i=0; i<N; ++i){
//             int v; 
//             ff.pop(v);
//         }    
//     ;});
// }

void bench_fifo3(benchmark::State& s){
    int N = s.range();
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
        }
    });

    t1.join();
    t2.join();
}

BENCHMARK(bench_fifo3)->Arg(1<<10);
// BENCHMARK(bench_fifo2)->Arg(1<<10);

BENCHMARK_MAIN();