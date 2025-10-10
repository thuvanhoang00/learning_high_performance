#pragma once
#include <chrono>
#include <iostream>

template<typename Fn>
auto time_it(const char* name, Fn&& fn)
{
    int count=10;
    int avg=0;
    while(count--){
        using namespace std::chrono;
        auto t0 = high_resolution_clock::now();
        fn();
        auto t1 = high_resolution_clock::now();
        auto ms = duration_cast<milliseconds>(t1-t0).count();
        avg += ms;
    }
    std::cout << name << ":" << avg/10 << " ms\n";
}