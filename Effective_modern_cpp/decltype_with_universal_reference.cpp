#include <deque>
#include <string>
#include <iostream>
template<typename Container, typename Index>
decltype(auto) authAndAccess(Container&& c, Index i){
    return std::forward<Container>(c)[i];
}

std::deque<std::string> makeStringDeque()
{
    return std::deque<std::string>{"1", "2", "3"};
}

int main(){
    auto s = authAndAccess(makeStringDeque(), 1);
    std::deque<std::string> queue{"a", "b", "c"};
    auto s2 = authAndAccess(queue, 0);
    s2 = "b";
    
    std::cout << queue[0] << std::endl;
    return 0;
}