#include <benchmark/benchmark.h>
#include "fifo2.h"
#include <thread>

void BM_fifo2(benchmark::State& s)
{
    int N = 1<<s.range(0);
    Fifo2<int> lfq(100);
    for(auto _ : s){
        std::jthread push(
            [&](){   
                for(int i=0; i<N; ++i){
                    lfq.push(i);
                }
            }
        );
        std::jthread pop(
            [&](){   
                for(int i=0; i<N; ++i){
                    int v;
                    lfq.pop(v);
                }
            }
        );
    }
}

void BM_fifo3(benchmark::State& s)
{
    int N = 1<<s.range(0);
    Fifo2<int> lfq(100);
    for(auto _ : s){
        std::jthread push(
            [&](){   
                for(int i=0; i<N; ++i){
                    lfq.push(i);
                }
            }
        );
        std::jthread pop(
            [&](){   
                for(int i=0; i<N; ++i){
                    int v;
                    lfq.pop(v);
                }
            }
        );
    }
}
BENCHMARK(BM_fifo2)->DenseRange(15, 20);
BENCHMARK(BM_fifo3)->DenseRange(15, 20);
BENCHMARK_MAIN();