#include <atomic>
#include <vector>

int main(){
    // std::atomic is non-copyable and non-moveable
    std::vector<std::atomic<int>> vec;
    // vec.reserve(4); // only allocate memory doesnt construct? -> why wont compile here
    // vec.resize(4); // will fail
    for(int i=0; i<4; i++){
        // vec.push_back(0); // still fail
        // vec.emplace_back(0); // fail again
        // vec.push_back(std::atomic<int>{0}); // fail too
        // vec.emplace_back(0); // failed
    }

    // FIX:
    std::vector<std::atomic<int>> vec2(4);// initialize with fixed-size
    for(int i=0; i<4; i++){
        vec2[i].store(0, std::memory_order_relaxed);
    }

    return 0;
}