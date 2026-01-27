#include "crtp.h"
#include <iostream>
void doSth(DenseVector<DynamicVector<int>>& vec){
    std::cout << vec.size();
}

int main(){
    DynamicVector<int> v(100);
    doSth(v);
    return 0;
}