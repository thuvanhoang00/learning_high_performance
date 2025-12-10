#include <algorithm>
#include <vector>
#include <iostream>

int main(){
    std::vector<int> vec = {1,2,3,4,3,3,5,2,4,0,5,9,7,8,6};
    // std::sort(vec.begin(), vec.end());
    std::ranges::sort(vec);
    auto it = std::ranges::binary_search(vec, 4);
    for(auto e : vec){
        std::cout << e << " ";
    }
    std::cout << std::boolalpha << it;

    auto it1 = std::ranges::lower_bound(vec, 4);
    if(it1 != vec.end()){
        auto idx = std::distance(vec.begin(), it1);
        std::cout << "\nidx: " << idx << "\n";
    }

    auto subrange = std::ranges::equal_range(vec, 4);
    if(subrange.begin() != subrange.end()){
        auto pos1 = std::distance(vec.begin(), subrange.begin());
        auto pos2 = std::distance(vec.begin(), subrange.end());
        std::cout << pos1 << " , " << pos2 << std::endl;
    }
    return 0;
}