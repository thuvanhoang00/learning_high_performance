#include <string_view>
#include <iostream>

void some_func(std::string_view s){
    std::cout << s << ", size: " << s.size();
}

int main(){
    some_func("Hello, world!\n"); // 14
    some_func("Hello, world!"); // 13
    some_func("Hello, world!\0"); // 13
    return 0;
}