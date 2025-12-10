#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <benchmark/benchmark.h>

struct LargeObject{
    std::string name_;
    std::string user_name_;
    std::string password_;
    std::string security_question_;
    std::string security_answer_;
    short scores_ = std::rand()%2;
    bool is_playing_;
};

struct AuthInfo{
    std::string user_name_;
    std::string password_;
    std::string security_question_;
    std::string security_answer_;
};

struct SmallObject{
    std::string name_;
    std::unique_ptr<AuthInfo> pAuthInfo_;
    short scores_ = std::rand()%2;
    bool is_playing_;
};

void BM_large_object(benchmark::State& s){
    const int N = 1<<s.range(0);
    std::vector<LargeObject> vec(N);
    for(auto _ : s){
        volatile int sum = 0;
        for(const auto& e : vec){
            sum += e.scores_;
        }
    }
}

void BM_small_object(benchmark::State& s){
    const int N = 1<<s.range(0);
    std::vector<SmallObject> vec(N);
    for(auto _ : s){
        volatile int sum = 0;
        for(const auto& e : vec){
            sum += e.scores_;
        }
    }
}

BENCHMARK(BM_large_object)->Arg(22);
BENCHMARK(BM_small_object)->Arg(22);

BENCHMARK_MAIN();
