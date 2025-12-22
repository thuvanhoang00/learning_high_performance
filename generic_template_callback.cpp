#include <iostream>
#include <unordered_map>
#include <vector>
#include <functional>
#include <typeindex>

struct Foo{
    std::unordered_map<std::type_index, std::vector<std::function<void(const void*)>>> cbs_;

    template<typename T>
    void add_cb(std::function<void(const void*)> cb){
        cbs_[std::type_index(typeid(T))].push_back(cb);
    }

    template<typename T>
    void call_cb(T data){
        if(cbs_.find(std::type_index(typeid(T))) != cbs_.end()){
            auto cb_list = cbs_[std::type_index(typeid(T))];
            for(auto cb : cb_list){
                cb(&data);
            }
        }
    }
};

struct Bar
{
    char a;
    char b;
    double c;
};

int main(){
    Foo f;

    f.add_cb<int>([](const void* a){
        auto cast_back_to_origin_value = *(static_cast<const int*>(a));
        std::cout << "int: " << cast_back_to_origin_value << std::endl;
    });

    f.add_cb<Bar>([](const void* arg){
        auto origin_val = *(static_cast<const Bar*>(arg));
        std::cout << "Bar: " << origin_val.a << ", " << origin_val.b << ", " << origin_val.c << std::endl;
    });

    f.call_cb<int>(1111);
    f.call_cb<Bar>({'h','l',3});
    return 0;
}