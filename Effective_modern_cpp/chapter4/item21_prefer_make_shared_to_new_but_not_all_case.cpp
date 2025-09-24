#include <memory>
void foo(){
    // may throw
}

struct Widget{
    int a;
};

void processWidget(std::shared_ptr<Widget> ptr, void(*f)()){

}

int main(){
    processWidget(std::shared_ptr<Widget>(new Widget), foo); // may leak memory if foo throw

    // to fix this:
    std::shared_ptr<Widget> ptr(new Widget);
    processWidget(std::move(ptr), foo); // OK, efficient and exception safety
    return 0;
}