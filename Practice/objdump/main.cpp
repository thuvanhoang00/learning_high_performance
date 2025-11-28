/*
Opt1:
1. man nm
2. nm -C a.out

Opt2:
1. objdump -t -C a.out
*/


#include <iostream>
class Foo{
public:
    int x=0;
};

int main(){
    // Foo f;
    // Foo f2;
    // f2 = f;
    // Foo f3=f2;
    // std::cout << f2.x;
    int x = 99;

    auto fooooooooo = [&](int c, int d){
        ++x;
    };
    fooooooooo(1,2);
    std::cout << x;
    return 0;
}