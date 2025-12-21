#pragma once
#include <iostream>
#include <string>
#include <functional>
#include "PPSManager.h"

class IView{
public:
    virtual ~IView() = default;
};

class View1 : public IView{
private:

public:
    View1() {}

    ~View1() {}

    void existed_function(std::string r_data){
        std::cout << "Call existed_function by data from Model " << r_data << std::endl;
    }
};

class OtherView : public IView{
private:

public:
    OtherView() {}

    ~OtherView() {}

    void other_function(std::string r_data){
        std::cout << "Other_function received data from Model " << r_data << std::endl;
    }
};