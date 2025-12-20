#include "PPSReader.h"


void PPSReader::start(){

}

void PPSReader::stop(){

}

void PPSReader::watch(std::string obj_path, std::function<void(std::string)> cb)
{
    subscribers_[obj_path] = cb;
}