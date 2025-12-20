#include "PPSWriter.h"
#include "PPSManager.h"

void PPSWriter::doWrite(std::string obj_path, std::string msg){
    PPSManager::instance().shared_data_ = msg;
}

void PPSWriter::start(){

}

void PPSWriter::stop(){

}