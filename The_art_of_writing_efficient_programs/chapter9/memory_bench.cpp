#include <benchmark/benchmark.h>
#include <cstdlib>
#include <cstddef>
#include <string.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <vector>
#include <algorithm>
#include <memory>
void BM_make_str_new(benchmark::State& state){
    const size_t NMax = state.range(0);
    for(auto _ : state){
        const size_t N = (rand()%NMax)+1;
        char* buf = new char[N];
        memset(buf, 0xab, N);
        delete[] buf;
    }
    state.SetItemsProcessed(state.iterations());
}

void BM_make_str_max(benchmark::State& state){
    const size_t NMax = state.range(0);
    char* buf = new char[NMax];
    for(auto _ : state){
        const size_t N = (rand()%NMax)+1;
        memset(buf, 0xab, N);
    }
    delete[] buf;
    state.SetItemsProcessed(state.iterations());
}

class Buffer{
    size_t size_;
    std::unique_ptr<char[]> buf_;
public:
    explicit Buffer(size_t N) : size_(N), buf_(new char[N]){}
    void resize(size_t N){
        if(N <= size_) return;
        char* new_buf = new char[N];
        memcpy(new_buf, get(), size_);
        buf_.reset(new_buf);
        size_ = N;
    }
    char* get(){return &buf_[0];};
};

void BM_make_str_buf(benchmark::State& state){
    const size_t NMax = state.range(0);
    Buffer buf(1);
    for(auto _ : state){
        const size_t N = (rand()%NMax) + 1;
        buf.resize(N);
        memset(buf.get(), 0xab, N);
    }
    state.SetItemsProcessed(state.iterations());
}

#define ARGS ->Arg(1<<20)->Threads(2);
BENCHMARK(BM_make_str_new) ARGS;
BENCHMARK(BM_make_str_max) ARGS;
BENCHMARK(BM_make_str_buf) ARGS;

BENCHMARK_MAIN();