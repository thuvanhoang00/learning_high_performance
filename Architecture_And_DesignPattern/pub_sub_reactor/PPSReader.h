#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <thread>
#include <atomic>
#include "PPSManager.h"
class PPSReader{
private:
    std::unordered_map<std::string, CallbackList> subscribers_; // 1 Object - Multi Viewers
    std::thread worker_;
    std::atomic<bool> is_running_;
private:
    void doRead(){
        while (is_running_)
        {
            for(auto e : subscribers_)
            {
                // e.second(PPSManager::instance().shared_data_);
                for(auto cb : e.second){
                    cb(PPSManager::instance().shared_data_);
                }
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
    void watch(std::string obj_path, ViewerCallback cb); 
    void start();
    void stop();
};