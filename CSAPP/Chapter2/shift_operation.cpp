#include <iostream>

int main()
{
    int lval = 0xfedcba98;
    std::cout <<  (lval<<32) << "\n"; // undefined behavior
    return 0;
}