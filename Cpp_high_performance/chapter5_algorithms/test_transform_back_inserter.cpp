#include <algorithm>
#include <vector>
#include <numeric>
#include <iostream>
int main(){
    std::vector<int> before(100);
    std::iota(before.begin(), before.end(), 0);
    // transform to new vector
    std::vector<int> new_vec;

    // if using transform with empty vector -> CRASH
    // std::ranges::transform(new_vew, [](int i){return i*i;});
    
    // use inserter instead
    std::ranges::transform(before, std::back_inserter(new_vec), [](int i){return i*i;});
    for(auto e : new_vec){
        std::cout << e << " ";
    }
    return 0;
}