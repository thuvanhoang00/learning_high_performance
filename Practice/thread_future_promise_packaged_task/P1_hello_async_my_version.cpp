#include <thread>
#include <future>
#include <iostream>
#include <benchmark/benchmark.h>

/*
1. Hello std::async
Goal: run a function asynchronously and get result.
Concepts: std::async, std::future, blocking vs deferred.
APIs: std::async, std::future::get, std::launch flags.
Difficulty: 1/5
Task: write int fib_async(int n) that computes fib(n) using std::async for recursive branches for n > threshold.
Hints: avoid spawning tasks for small n (use threshold). beware std::launch::deferred.
Test: fib_async(30) returns 832040. Measure performance vs single-threaded.
*/

#define THRESHOLD 5

int fib_single_thread(int n){
    if(n==0) return 0;
    if(n==1) return 1;
    if(n==2) return 1;
    return fib_single_thread(n-1) + fib_single_thread(n-2);
}

int fib_async(int n){
    if(n==0) return 0;
    if(n==1) return 1;
    if(n==2) return 1;
    if(n<=THRESHOLD){
        return fib_async(n-1)+fib_async(n-2);
    }
    auto fut1 = std::async(std::launch::async, fib_async, n-1);
    auto fut2 = std::async(std::launch::async, fib_async, n-2);
    return fut1.get() + fut2.get();
}

void BM_fib_single_thread(benchmark::State& s){
    int N = s.range(0);
    volatile int res = 0;
    for(auto _ : s){
        benchmark::DoNotOptimize(fib_single_thread(N));
    }
}

void BM_fib_async(benchmark::State& s){
    int N = s.range(0);
    volatile int res = 0;
    for(auto _ : s){
        res = fib_async(N);
    }
}

BENCHMARK(BM_fib_async)->Arg(15);
BENCHMARK(BM_fib_single_thread)->Arg(15);
BENCHMARK_MAIN();
