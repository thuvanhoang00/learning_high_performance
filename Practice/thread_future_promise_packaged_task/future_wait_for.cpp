#include <future>
#include <iostream>
#include <thread>

int doSth(){
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 1;
}

int main(){
    std::future<int> fut = std::async(std::launch::async, doSth);

    std::future_status stt = fut.wait_for(std::chrono::seconds(1));
    if(stt == std::future_status::ready){
        std::cout << "get = " << fut.get();
    }
    else{
        std::cout << "not done yet\n";
    }

    auto fut2 = std::async(doSth);
    return 0;
}


