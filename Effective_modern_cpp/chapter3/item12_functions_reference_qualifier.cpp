#include <iostream>
#include <string>

struct Widget{
    int a;
    void doWork()& // applies only *this is lvalue; 
    {
        std::cout << "a: " << a << std::endl;
    }

    void doWork()&& // if *this is rvalue
    {
        std::cout << "temp's a: " << a << std::endl; 
    }

    void doWork_const() const
    {
        std::cout << "const a: " << a << std::endl;
    }
};

Widget makeWidget(){
    return Widget();
}


int main()
{
    makeWidget().doWork();
    const Widget w{};
    w.doWork_const();
    return 0;
}