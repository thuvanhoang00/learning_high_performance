#include "memory_pool.h"
struct MyStruct{
    int d_[3];
};

int main(int, char**){
    using namespace thu;
    MemPool<double> prim_bool(50);
    MemPool<MyStruct> struct_pool(50);
    for(auto i=0; i<50; ++i){
        auto p_ret = prim_bool.allocate(i);
        auto s_ret = struct_pool.allocate(MyStruct{i, i+1, i+2});

        std::cout << "prim_elem:" << *p_ret << " alloc at: " << p_ret << std::endl;
        std::cout << "struct_elem:" << s_ret->d_[0] << ", " << s_ret->d_[1] << ", " << s_ret->d_[2] << " alloc at: " << s_ret << std::endl;

        if(i%5 == 0){
            std::cout << "deallocating prim_elem: " << *p_ret << " from: " << p_ret << std::endl;
            prim_bool.deallocate(p_ret);

            std::cout << "deallocating struct elem: " << s_ret->d_[0] << ", " << s_ret->d_[1]
                      << ", " << s_ret->d_[2] << " from: " << s_ret << std::endl;
            struct_pool.deallocate(s_ret);
        }
    }

    return 0;

}