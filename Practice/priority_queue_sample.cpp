#include <iostream>
#include <vector>
#include <array>
#include <queue>
#include <algorithm>
std::vector<int> makeVector(){
    std::vector<int> ret(100);
    std::generate(ret.begin(), ret.end(), []()->int{return rand()%100;});
    return ret;
}
int main(){
    std::vector<int> rank = makeVector();
    std::vector<int> top_ten;
    // this queue keep top ten value for ranking
    // greater<int> is min-queue
    std::priority_queue<int, std::vector<int>, std::greater<int>> max_queue;

    // for(auto e : rank){
    //     std::cout << e << " ";
    // }

    int count = 0;
    for(auto e : rank){
        if(count<10){
            max_queue.push(e);
            count++;
        }
        else{
            if(e > max_queue.top()){
                max_queue.pop();
                max_queue.push(e);
            }
        }
    }

    while(!max_queue.empty()){
        auto e = max_queue.top();
        max_queue.pop();
        top_ten.push_back(e);
    }
    std::reverse(top_ten.begin(), top_ten.end());
    for(auto e : top_ten){
        std::cout << e << " ";
    }
    std::cout << "-----\n\n";
    return 0;
}