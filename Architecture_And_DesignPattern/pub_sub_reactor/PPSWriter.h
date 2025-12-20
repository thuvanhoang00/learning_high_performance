#pragma once
#include <string>

class PPSWriter{
private:
public:
    void start();
    void stop();
    void doWrite(std::string obj_path, std::string msg);
};