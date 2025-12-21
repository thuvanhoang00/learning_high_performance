#include "PPSReader.h"


void PPSReader::start(){

}

void PPSReader::stop(){

}

void PPSReader::watch(std::string obj_path, ViewerCallback cb)
{
    subscribers_[obj_path].push_back(std::move(cb));
}