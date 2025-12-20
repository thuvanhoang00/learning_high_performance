#pragma once
#include <functional>
#include <memory>
#include <string>
class PPSWriter;
class PPSReader;
class PPSManager{
private:
    std::unique_ptr<PPSWriter> pWriter_;
    std::unique_ptr<PPSReader> pReader_;
    PPSManager();
    ~PPSManager();
public:
    static PPSManager& instance(){
        static PPSManager instance;
        return instance;
    }
    void init();
    void publish(std::string obj_path, std::string msg);
    void addViewer(std::string obj_path, std::function<void(std::string)> cb);
    std::string shared_data_; 
};