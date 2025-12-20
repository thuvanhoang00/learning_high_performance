#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <thread>
#include <atomic>
#include "PPSManager.h"
class PPSReader{
private:
    std::unordered_map<std::string, std::function<void(std::string received_data)>> subscribers_;
    std::thread worker_;
    std::atomic<bool> is_running_;
private:
    void doRead(){
        while (is_running_)
        {
            for(auto e : subscribers_)
            {
                e.second(PPSManager::instance().shared_data_);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
    }
public:
    PPSReader(){
        is_running_ = true;
        worker_ = std::move(std::thread(&PPSReader::doRead, this));
    }
    ~PPSReader(){
        is_running_ = false;
        if(worker_.joinable())
        {
            worker_.join();
        }
    }
    void watch(std::string obj_path, std::function<void(std::string)> cb);
    void start();
    void stop();
};