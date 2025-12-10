#include <span>
#include <iostream>
#include <vector>

void usingSpan(std::span<int> buffer){
    for(auto& e : buffer){
        e++;
        std::cout << e << " ";
    }
    std::cout << std::endl;
}

int main(){
    int arr[5] = {1,2,3,4,5};
    std::vector<int> v = {5,4,3,2,1,0,-1};
    usingSpan(arr);
    usingSpan(v);
    return 0;
}