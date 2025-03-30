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

void SPSC_mtx_test(size_t size)
{
    SPSC spsc(size);
    std::thread t1(&SPSC::produce_mtx, &spsc);
    std::thread t2(&SPSC::consume_mtx, &spsc);
    t1.join();
    t2.join();
}

void SPSC_sp_test(size_t size)
{
    SPSC spsc(size);
    std::thread t1(&SPSC::produce_sp, &spsc);
    std::thread t2(&SPSC::consume_sp, &spsc);
    t1.join();
    t2.join();
}

static void benchmark_SPSC_cv(benchmark::State& s){
    int size = 1000000;
    
    for(auto state : s){
        SPSC_cv_test(size);
    }
}



static void benchmark_SPSC_mtx(benchmark::State& s)
{
    int size = s.range(0);
    for(auto state : s){
        SPSC_mtx_test(size);
    }
}

static void benchmark_SPSC_sp(benchmark::State& s)
{
    int size = s.range(0);
    for(auto state : s){
        SPSC_sp_test(size);
    }
}

// BENCHMARK(benchmark_SPSC_cv);
BENCHMARK(benchmark_SPSC_mtx)->DenseRange(20,25);
BENCHMARK(benchmark_SPSC_sp)->DenseRange(20,25);

BENCHMARK_MAIN();