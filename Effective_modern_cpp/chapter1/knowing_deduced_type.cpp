#include <string>
#include <iostream>
// 1. Compiler diagnostics -> leads to compilation problems then get deduced type
template<typename T>
class TD;
// TD<decltype(x)> xType;

int main()
{
    // 2. IDE only helps on simple type // not reliable
    const int theAnswer = 42;
    auto x = theAnswer; // VSC show x is int 
    auto y = &theAnswer; // y is const int*
    return 0;

    // 3. std::type_info::name // not reliable

    // 4. Boost::TypeIndedx
    // type_id_with_cvr<T>().pretty_name()
}