#pragma once
#include "PPSManager.h"
class Model{
private:

public:
    static void onModelInfoAvailable()
    {
        PPSManager::instance().publish("/pps/thuh4", "Hello world from Model!");
    }
};