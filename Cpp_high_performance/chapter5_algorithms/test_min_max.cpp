#include <algorithm>
#include <iostream>
#include <vector>
int main(){
    const auto y_max = 100;
    std::vector<int> v(100);
    std::ranges::generate(v, std::rand);

    const auto [min, max] = std::ranges::minmax(v);
    std::cout << min << " " << max;

    return 0;
}