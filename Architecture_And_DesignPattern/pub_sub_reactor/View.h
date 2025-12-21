#pragma once
#include <iostream>
#include <string>
#include <functional>
#include "PPSManager.h"

class IView{
public:
    virtual ~IView() = 0;
};

class View1 : public IView{
private:

public:
    View1() {}

    ~View1() override {}

    void existed_function(std::string r_data){
        std::cout << "Call existed_function by data from Model " << r_data << std::endl;
    }
};