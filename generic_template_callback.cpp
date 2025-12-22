#include <iostream>
#include <unordered_map>
#include <vector>
#include <functional>


struct Foo{
    std::vector<std::function<void(const void*)>> cbs_;

    void add_cb(std::function<void(const void*)> cb){
        cbs_.push_back(cb);
    }

    void call_cb(){
        for(auto cb : cbs_){
            int x = 1;
            cb(&x);
        }
    }
};


int main(){
    Foo f;

    auto int_cb = [](int x){
        std::cout << x << std::endl;
    };

    f.add_cb([](const void* a){
        std::cout << static_cast<const int*>(a);
    });
    
    f.call_cb();
    return 0;
}