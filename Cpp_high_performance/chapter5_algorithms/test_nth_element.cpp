#include <vector>
#include <algorithm>
#include <iostream>

int main(){
    std::vector<int> v{6,3,2,7,4,1,5};
    auto it = v.begin() + v.size()/2;
    auto left = it-1;
    auto right = it+2;
    std::nth_element(v.begin(), left, v.end());
    std::nth_element(v.begin(), right, v.end());
    std::cout << *left << std::endl;
    std::cout << *right << std::endl;
    std::partial_sort(left, right, v.end());
    for(auto e: v){
        std::cout << e << " ";
    }
    std::cout << "\n";
    return 0;
}