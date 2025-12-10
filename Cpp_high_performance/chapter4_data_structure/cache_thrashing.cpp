#include <array>
#include <iostream>

/*
RUN: perf stat -e cycles,instructions,L1-dcache-load-misses,L1-dcache-loads ./a.out
*/

constexpr int kL1CacheSize = 32768;
constexpr int kSize = kL1CacheSize/sizeof(int);
using MatrixType = std::array<std::array<int, kSize>, kSize>;

void do_bench(MatrixType* mt){
    for(int i=0; i<kSize; ++i){
        volatile int count=rand()%10;
        for(int j=0; j<kSize; ++j){
            (*mt)[i][j] = count;
        }
    }
}

int main(){
    MatrixType* p_mt = new MatrixType();
    do_bench(p_mt);   
    return 0;
}