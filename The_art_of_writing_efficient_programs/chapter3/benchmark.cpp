#define MCA_START __asm volatile("# LLVM-MCA-BEGIN");
#define MCA_END __asm volatile("# LLVM-MCA-END");
#include <vector>
#include <random>
int main(){
    constexpr int N = 10000;
    std::vector<unsigned long> v1(N), v2(N);
    for(size_t i = 0; i < N; i++){
        v1[i]=rand();
        v2[i]=rand();
    }
    unsigned long* p1 = v1.data();
    unsigned long* p2 = v2.data();
    volatile unsigned long al=0;
    for(size_t i = 0; i < N; ++i){
MCA_START
        al += p1[i] + p2[i];
MCA_END
    }
    return 0;
}