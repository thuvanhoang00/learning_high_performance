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

    /*
    *   Spinlock version 2
    *   Using 1 atomic
    */
void SPSC_sp2_test(size_t size)
{
    SPSC spsc(size);
    std::thread t1(&SPSC::produce_sp2, &spsc);
    std::thread t2(&SPSC::consume_sp2, &spsc);
    t1.join();
    t2.join();
}

static void benchmark_SPSC_cv(benchmark::State& s){
    int size = 1000;
    
    for(auto state : s){
        SPSC_cv_test(size);
    }
}

static void benchmark_SPSC_mtx(benchmark::State& s)
{
    size_t size = 100000;
    for(auto state : s){
        SPSC_mtx_test(size);
    }
}

static void benchmark_SPSC_sp(benchmark::State& s)
{
    size_t size = 100000;
    for(auto state : s){
        SPSC_sp_test(size);
    }
}

static void benchmark_SPSC_sp2(benchmark::State& s)
{
    size_t size = 100000;
    for(auto state : s){
        SPSC_sp2_test(size);
    }
}

// BENCHMARK(benchmark_SPSC_cv);
BENCHMARK(benchmark_SPSC_mtx);
BENCHMARK(benchmark_SPSC_sp);
// BENCHMARK(benchmark_SPSC_sp2);

BENCHMARK_MAIN();