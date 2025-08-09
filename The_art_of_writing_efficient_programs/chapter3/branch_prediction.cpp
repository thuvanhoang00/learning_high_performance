#include "benchmark/benchmark.h"
#include <memory>
#include <vector>
#include <cstring>
void branch(benchmark::State& state){
    const int N = state.range(0);
    std::vector<unsigned long> v1(N), v2(N);
    std::vector<int> c1(N);
    for(size_t i=0; i<N; ++i){
        v1[i] = rand();
        v2[i] = rand();
        c1[i] = rand() & 1;
    }
    unsigned long* p1 = v1.data();
    unsigned long* p2 = v2.data();
    volatile int* b1 = c1.data();
    for(auto _ : state){
        unsigned long a1 = 0, a2 = 0;
        for(size_t i = 0; i<N; ++i){
            if(b1[i]){
                a1 += p1[i];
            }
            else{
                a2 += p2[i];
            }
        }
        benchmark::DoNotOptimize(a1);
        benchmark::DoNotOptimize(a2);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(branch)->Arg(1<<20)->Iterations(1);
BENCHMARK_MAIN();