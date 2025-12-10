#include <algorithm>
#include <vector>
#include <iostream>

int main(){
    std::vector<int> vec = {1,2,3,4,0,5,9,7,8,6};
    // std::sort(vec.begin(), vec.end());
    std::ranges::sort(vec);

    for(auto e : vec){
        std::cout << e << " ";
    }

    return 0;
}