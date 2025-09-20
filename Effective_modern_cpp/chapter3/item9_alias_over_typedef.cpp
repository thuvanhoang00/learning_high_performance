#include <list>

struct Widget;
struct MyAlloc;
template<typename T>
using MyAllocList = std::list<T, MyAlloc<T>>;
MyAllocList<Widget> lw; // -> Use in client code


template<typename T>
struct MyAllocList{
    typedef std::list<T, MyAlloc<T>> type;
};
MyAllocList<Widget>::type lw ; // -> use in client code