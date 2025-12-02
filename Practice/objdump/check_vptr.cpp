#include <iostream>
#include <memory>

// what am i doing?
// -> checking vptr

// how to check
// -> add two classes with virtual foo()

// can only provee that vtable exists by : nm -C a.out | grep "vtable" -> will show vtable for class that have virtual function
// OR : objdump -d -S -C a.out | grep "vtable"

// BEST SOLUTION HERE:
// 1. clang++ -Xclang -fdump-record-layouts -c check_vptr.cpp
// 2. g++ -fdump-lang-class check_vptr.cpp


class Foo{
private:

public:
    virtual void foo() {std::cout << "Foo->foo()\n";}
    virtual ~Foo() = default;
};


class ChildFoo : public Foo{
public:
    // virtual void chillfoo(){}
    virtual void foo() override {std::cout << "ChildFoo->foo()\n";}
};


int main(){

    // std::unique_ptr<Foo> fptr = std::make_unique<Foo>();
    Foo* fptr = new Foo();
    fptr->foo();
    // std::unique_ptr<Foo> cfptr = std::make_unique<ChildFoo>();
    Foo* cfptr = new ChildFoo();
    cfptr->foo();
    delete fptr; delete cfptr;
    return 0;
}