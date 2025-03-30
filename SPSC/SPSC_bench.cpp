#include <benchmark/benchmark.h>
#include "SPSC.h"

void SPSC_cv_test(size_t size)
{
    SPSC spsc(size);
    std::thread t1(&SPSC::produce_cv, &spsc);
    std::thread t2(&SPSC::consume_cv, &spsc);
    t1.join();
    t2.join();
}

static void benchmark_SPSC_cv(benchmark::State& s){
    int size = 1000;
    
    for(auto state : s){
        SPSC_cv_test(size);
    }
}

void SPSC_mtx_test(size_t size)
{
    SPSC spsc(size);
    std::thread t1(&SPSC::produce_mtx, &spsc);
    std::thread t2(&SPSC::consume_mtx, &spsc);
    t1.join();
    t2.join();
}

static void benchmark_SPSC_mtx(benchmark::State& s)
{
    int size = 10000;
    for(auto state : s){
        SPSC_mtx_test(size);
    }
}

BENCHMARK(benchmark_SPSC_cv);
BENCHMARK(benchmark_SPSC_mtx);
BENCHMARK_MAIN();