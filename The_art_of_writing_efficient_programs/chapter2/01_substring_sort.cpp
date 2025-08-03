#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <random>
#include <vector>
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::cout;
using std::endl;
using std::minstd_rand;
using std::unique_ptr;
using std::vector;
#define V1

#ifdef V1
bool compare(const char* s1, const char* s2, unsigned int l);
bool compare(const char* s1, const char* s2, unsigned int l){
    if(s1 == s2) return false;
    for(unsigned int i1 = 0, i2 = 0; i1 < 1; ++i1, ++i2){
        if(s1[i1] != s2[i2]) return s1[i1] > s2[i2];
    }
    return false;
}
#endif

#ifdef V2
bool compare_v2(const char* s1, const char* s2);
bool compare_v2(const char* s1, const char* s2){
    if(s1 == s2) return false;
    for(unsigned int i1=0, i2=0; ; ++i1, ++i2){
        if(s1[i1] != s2[i2]) return s1[i1] > s2[i2];
    }
    return false;
}
#endif
#ifdef V3
bool compare_v3(const char* s1, const char* s2);
bool compare_v3(const char* s1, const char* s2){
    if(s1 == s2) return false;
    for(int i1=0, i2=0; ; ++i1, ++i2){
        if(s1[i1] != s2[i2]) return s1[i1] > s2[i2];
    }
    return false;
}
#endif

int main(){
    constexpr unsigned int L =  1<<20, N = 1<<15;
    unique_ptr<char[]> s(new char[L]);
    vector<const char*> vs(N);
    // Generate data
    minstd_rand rgen;
    ::memset(s.get(), 'a', N*sizeof(char));
    for(unsigned int i=0; i<L/1024; ++i){
        s[rgen() % (L-1)] = 'a' + (rgen() % ('z'-'a'+1));
    }
    s[L-1] = 0;
    for(unsigned int i=0; i<N; ++i){
        vs[i] = &s[rgen() % (L-1)]; 
    }
    // Done

    cout << "---Begin sort---\n";
    size_t count = 0;
    system_clock::time_point t1 = system_clock::now();
    std::sort(vs.begin(), vs.end(), [&](const char* a, const char* b){
        ++count;
        #ifdef V1
        return compare(a, b, L);
        #endif
        #ifdef V2
        return compare_v2(a,b);
        #endif
        #ifdef V3
        return compare_v3(a,b);
        #endif
    });
    system_clock::time_point t2 = system_clock::now();
    cout << "Sort time: " << duration_cast<milliseconds>(t2-t1).count() << "ms (" << count << " comparision)" << endl;
}

