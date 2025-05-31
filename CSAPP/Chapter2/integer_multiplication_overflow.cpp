#include <iostream>

int tmult_ok(int x, int y){
    int p = x*y;
    return !x || p/x==y;
}

int tmult64_ok(int x, int y){
    int64_t p = static_cast<int64_t>(x)*y;
    return p == static_cast<int>(p);
}
int main()
{
    std::cout << tmult64_ok(223231,99999999);
    return 0;
}