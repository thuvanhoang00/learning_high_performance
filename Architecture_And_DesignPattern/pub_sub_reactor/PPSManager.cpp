#include "PPSManager.h"
#include "PPSWriter.h"
#include "PPSReader.h"
#include <iostream>

PPSManager::PPSManager(){
    pWriter_ = std::make_unique<PPSWriter>();
    pReader_ = std::make_unique<PPSReader>();
}

PPSManager::~PPSManager()
{
    pWriter_->stop();
    pReader_->stop();
}

void PPSManager::init(){
    pWriter_->start();
    pReader_->start();
}
void PPSManager::publish(std::string obj_path, std::string msg)
{
    // std::cout << "Writer to object: " << obj_path << " - Msg: " << msg << std::endl;
    pWriter_->doWrite(obj_path, msg);
}
void PPSManager::addViewer(std::string obj_path, std::function<void(std::string r_data)> cb)
{
    pReader_->watch(obj_path, cb);
}