#include "common/fifo3.h"
#include "common/lockfreequeue.h"
#include <benchmark/benchmark.h>
#include <thread>

void BM_lockfreequeue(benchmark::State& s){
    int N = s.range(0);

    for(auto _ : s){
        LockFreeQueue<int> lfq;
        std::jthread push([N, &lfq](){
            for(int i=0; i<N; ++i){
                benchmark::DoNotOptimize(lfq.push(i));
            }
        });
        std::jthread pop([N, &lfq](){
            for(int i=0; i<N; ++i){
                int v;
                benchmark::DoNotOptimize(lfq.pop(v));
            }
        });
    }
    
}

void BM_FiFo3(benchmark::State& s){
    int N = s.range(0);
    Fifo3<int> lfq(N);

    for(auto _ : s){
        std::jthread push([N, &lfq](){
            for(int i=0; i<N; ++i){
                benchmark::DoNotOptimize(lfq.push(i));
            }
        });
        std::jthread pop([N, &lfq](){
            for(int i=0; i<N; ++i){
                int v;
                benchmark::DoNotOptimize(lfq.pop(v));
            }
        });
    }
}

BENCHMARK(BM_lockfreequeue)->DenseRange(20,25);
BENCHMARK(BM_FiFo3)->DenseRange(20,25);
BENCHMARK_MAIN();