#include <benchmark/benchmark.h>
#include <cstdlib>
#include <cstddef>
#include <string.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <vector>
#include <algorithm>
#define REPEAT2(x)  x x
#define REPEAT4(x)  REPEAT2(x) REPEAT2(x)
#define REPEAT8(x)  REPEAT4(x) REPEAT4(x)
#define REPEAT16(x) REPEAT8(x) REPEAT8(x)
#define REPEAT32(x) REPEAT16(x) REPEAT16(x)
#define REPEAT(x)   REPEAT32(x)

void print_sorted(std::vector<int> v){
    std::sort(v.begin(), v.end());
}

template<typename T>
void print_sorted_cpy(std::vector<T> v){
    std::sort(v.begin(), v.end(), [](const T& t1, const T& t2){
        return t1<t2;
    });
}

template<typename T>
void print_sorted(const std::vector<T>& v){
    std::vector<const T*> vp;
    vp.reserve(v.size());
    for(const T& x : v){
        vp.push_back(&x);
    }
    std::sort(vp.begin(), vp.end(), [](const T* a, const T* b){ return *a < *b;});
}

template<typename T>
void BM_sort_cpy(benchmark::State& state){
    const size_t N = state.range(0);
    std::vector<T> v0(N);
    for(T& x : v0){
        x = T();
    }
    std::vector<T> v(N);
    for(auto _ : state){
        v = v0;
        print_sorted_cpy(v);
    }
    state.SetItemsProcessed(state.iterations()*N);
}

template<typename T>
void BM_sort_ptr(benchmark::State& state){
    const size_t N = state.range(0);

    std::vector<T> v0(N);
    for(auto& x : v0){
        x = T();
    }
    std::vector<T> v(N);
    for(auto _ : state){
        v = v0;
        print_sorted(v);
    }
    state.SetItemsProcessed(state.iterations()*N);
}
enum class FooEnum{
    A,
    B,
    C,
    D
};

struct Foo1{
    double a;
    double b;
    int arr[100] = {};
};

struct Foo{
    int a;
    int b;
    double c;
    char d;
    long e;
    long e1;
    long e2;
    long e3;
    long e4;
    long e5;
    FooEnum azzzzz;
    Foo1 foo1;
    friend bool operator<(const Foo& a, const Foo& b){
        return a.a < b.a;
    }
};

#define ARGS ->RangeMultiplier(2)->Range(1<<10, 1<<20)

// BENCHMARK_TEMPLATE1(BM_read_seq, unsigned long) ARGS;
// BENCHMARK_TEMPLATE1(BM_read_seq, __m256i) ARGS;
// BENCHMARK_TEMPLATE1(BM_write_seq, unsigned long) ARGS;
BENCHMARK_TEMPLATE1(BM_sort_ptr, Foo) ARGS;
BENCHMARK_TEMPLATE1(BM_sort_cpy, Foo) ARGS;
// BENCHMARK(BM_sort_cpy) ARGS;
BENCHMARK_MAIN();