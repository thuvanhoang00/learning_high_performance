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

/* GOOGLE REVISED
#include <iostream>
#include <unordered_map>
#include <vector>
#include <functional>
#include <typeindex>

struct Foo {
    // Internal storage: Still generic (void*) because the container must hold mixed types
    using GenericCallback = std::function<void(const void*)>;
    std::unordered_map<std::type_index, std::vector<GenericCallback>> cbs_;

    // 1. IMPROVEMENT: Take a TYPED callback from the user
    template<typename T>
    void add_cb(std::function<void(const T&)> user_cb) { 
        
        // 2. IMPROVEMENT: Create a "Thunk" (Wrapper) 
        // We wrap the user's safe function inside a generic lambda that handles the casting
        GenericCallback wrapper = [user_cb](const void* data) {
            // We know this cast is safe because we keyed it by typeid(T)
            const T* typedData = static_cast<const T*>(data);
            user_cb(*typedData); 
        };

        cbs_[typeid(T)].push_back(wrapper);
    }

    template<typename T>
    void call_cb(const T& data) { // Pass by const ref to avoid copy
        auto it = cbs_.find(typeid(T));
        if (it != cbs_.end()) {
            // 3. IMPROVEMENT: Iterate by Reference (const auto&)
            // Your original code did "auto cb_list =", which COPIES the whole vector!
            for (const auto& cb : it->second) {
                cb(&data);
            }
        }
    }
};

struct Bar {
    char a;
    char b;
    double c;
};

int main() {
    Foo f;

    // USER EXPERIENCE: No more void* casting! Clean and safe.
    f.add_cb<int>([](const int& val) {
        std::cout << "int: " << val << std::endl;
    });

    f.add_cb<Bar>([](const Bar& val) {
        std::cout << "Bar: " << val.a << ", " << val.b << ", " << val.c << std::endl;
    });

    f.call_cb<int>(1111);
    f.call_cb<Bar>({'h', 'l', 3.0});

    return 0;
}
*/