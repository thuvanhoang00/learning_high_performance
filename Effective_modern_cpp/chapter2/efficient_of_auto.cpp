#include "benchmark/benchmark.h"
#include <functional>
#include <memory>

struct Widget{
    Widget(int a, int b, int c) : a_(a), b_(b), c_(c) {}
    int a_=1;
    int b_=2;
    int c_=3;
};


void BM_function(benchmark::State& state){
    const unsigned N = state.range(0);
    for(auto _ : state){
        std::function<bool(const std::unique_ptr<Widget>&, const std::unique_ptr<Widget>&)>
            derefUPLess = [](const std::unique_ptr<Widget>& a, const std::unique_ptr<Widget>& b){
                return a->a_ < b->b_;
            };
        benchmark::DoNotOptimize(derefUPLess(std::make_unique<Widget>(1,2,3), std::make_unique<Widget>(2,3,1)));
    }
}

void BM_auto(benchmark::State& state){
    const unsigned N = state.range(0);
    for(auto _ : state){
        auto derefUPLess = [](const std::unique_ptr<Widget>& a, const std::unique_ptr<Widget>& b){
            return a->a_ < b->b_;
        };
        benchmark::DoNotOptimize(derefUPLess(std::make_unique<Widget>(1,2,3), std::make_unique<Widget>(2,3,1)));
    }
}
BENCHMARK(BM_function)->Arg(1<<30);
BENCHMARK(BM_auto)->Arg(1<<30); // auto is slightly better than std::function


#include <unordered_map>
#include <string>

void do_loop_with_pair(unsigned N){
    std::unordered_map<std::string, int> map; // pair in hash table is std::pair<const string, int>
    for(int i=0; i<N; ++i){
        map.insert({"a", 1});
    }
    // loop-through using pair
    for(const std::pair<std::string, int>& p : map)
    {
        std::string s = p.first;
        int i = p.second;
        i=2;
        s="b";
    }
}

void do_loop_with_auto_pair(unsigned N){
    std::unordered_map<std::string, int> map; // pair in hash table is std::pair<const string, int>
    for(int i=0; i<N; ++i){
        map.insert({"a", 1});
    }
    // loop-through using auto
    for(const auto& p : map)
    {
        std::string s = p.first;
        int i = p.second;
        i=2;
        s="b";
    }
}

void BM_std_pair(benchmark::State& state){
    const unsigned N = state.range(0);
    for(auto _ : state){
        do_loop_with_pair(N);
    }
}

void BM_auto_pair(benchmark::State& state){
    const unsigned N = state.range(0);
    for(auto _ : state){
        do_loop_with_auto_pair(N);
    }
}
BENCHMARK(BM_std_pair)->Arg(1<<20);
BENCHMARK(BM_auto_pair)->Arg(1<<20);

BENCHMARK_MAIN();

