#include <iostream>
// Protected from Base class
// still has effect on Child class

// RUN: clang -Xclang -fdump-record-layouts -c protected_inherit.cpp >> a.log
/*
OUTPUT:
*** Dumping AST Record Layout
         0 | class Base
         0 |   int x
           | [sizeof=4, dsize=4, align=4,
           |  nvsize=4, nvalign=4]

*** Dumping AST Record Layout
         0 | class Child
         0 |   class Base (base)
         0 |     int x
           | [sizeof=4, dsize=4, align=4,
           |  nvsize=4, nvalign=4]

*/

// Kill process : kill -SIGKILL [pid]

class Base{
protected: 
    int x;
};

class Child : public Base{

};


int main(){
    std::cout << sizeof(Base) << std::endl;
    std::cout << sizeof(Child) << std::endl;
    while (true)
    {
      /* code */
    }
    
    return 0;
}