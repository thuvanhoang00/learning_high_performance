#include <algorithm>
#include <array>
#include <iostream>

int main(){
    std::array<int, 10> arr{0,1,2,3,4,5,6,7,8,9};
    auto new_begin = std::next(arr.begin(), 4); // points to the 5th element
    // move first 4 element to the end of array->should be: 4,5,6,7,8,9,0,1,2,3
    std::rotate(arr.begin(), new_begin, arr.end());
    for(auto e : arr){
        std::cout << e << " "; 
    }
    std::cout << "\n";
    
    return 0;
}