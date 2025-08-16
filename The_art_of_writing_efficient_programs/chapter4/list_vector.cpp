#include <benchmark/benchmark.h>
#include <cstdlib>
#include <cstddef>
#include <vector>
#include <list>
#include <string.h>
#include <immintrin.h>
#include <emmintrin.h>
#define REPEAT2(x)  x x
#define REPEAT4(x)  REPEAT2(x) REPEAT2(x)
#define REPEAT8(x)  REPEAT4(x) REPEAT4(x)
#define REPEAT16(x) REPEAT8(x) REPEAT8(x)
#define REPEAT32(x) REPEAT16(x) REPEAT16(x)
#define REPEAT(x)   REPEAT32(x)


template<class Word>
void BM_write_vector(benchmark::State& state){
    const size_t size = state.range(0);
    std::vector<Word> c(size);
    Word x = {};
    for(auto _ : state){
        for(auto it = c.begin(), it0 = c.end(); it != it0;){
            REPEAT(benchmark::DoNotOptimize(*it++ = x);)
        }
        benchmark::ClobberMemory();
    }
}

template<class Word>
void BM_write_list(benchmark::State& state){
    const size_t size = state.range(0);
    std::list<Word> c(size);
    Word x = {};
    for(auto _ : state){
        for(auto it = c.begin(), it0 = c.end(); it != it0;){
            REPEAT(benchmark::DoNotOptimize(*it++ = x);)
        }
        benchmark::ClobberMemory();
    }
}
#define ARGS ->RangeMultiplier(2)->Range(1<<10, 1<<20)

BENCHMARK_TEMPLATE1(BM_write_vector, unsigned long) ARGS;
BENCHMARK_TEMPLATE1(BM_write_list, unsigned long) ARGS;
BENCHMARK_MAIN();