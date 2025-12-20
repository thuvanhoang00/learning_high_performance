#pragma once
#include <iostream>
#include <string>
#include <functional>
#include "PPSManager.h"
class View{
private:

public:
    View()
    {
        PPSManager::instance().addViewer("/pps/thuh4", [this](std::string r_data){
            std::cout << "View receives data: " << r_data;
        });
    }
};