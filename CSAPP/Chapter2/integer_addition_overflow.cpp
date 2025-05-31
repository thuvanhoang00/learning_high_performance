#include <iostream>
#include <limits.h>

int uadd_ok(unsigned x, unsigned y){
    if((x+y) < x) return 0;
    return 1;
}

int tadd_ok(int x, int y){
    int sum = x+y;
    int negative_overflow = (x < 0 && y < 0) ? (sum > x) : 0;
    int positive_overflow = (x > 0 && y > 0) ? (sum < x) : 0;
    return !negative_overflow && !positive_overflow;
}

int main()
{
    unsigned a = UINT_MAX;
    unsigned b = 100;
    std::cout << "is adding unsigned ok: " << uadd_ok(a,b) << "\n";

    int x = INT_MAX;
    int y = 100;
    std::cout << "is adding signed ok: " << tadd_ok(x, y) << "\n";
    int m = INT_MIN;
    int n = -100;
    std::cout << "is adding signed ok: " << tadd_ok(m, n) << "\n";
    return 0;
}