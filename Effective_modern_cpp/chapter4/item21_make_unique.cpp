#include <memory>

namespace thu{
    template<typename T,typename... Args>
    std::unique_ptr<T> make_unique(Args&& ...params)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(params)...));
    }
}

struct Widget{
    int a;
};

int main(){
    std::unique_ptr<Widget> ptr(thu::make_unique<Widget>());
    std::unique_ptr<Widget> ptr2(std::make_unique<Widget>());
    return 0;
}