#pragma once
#include "external_drawing_lib.h"
#include <iostream>
class OpenGLCirleDraw{
public:
    void draw(){
        std::cout << "using OpenGLCircleDraw\n";
    }
};


class BoostCirleDraw{
public:
    void draw(){
        std::cout << "using BoostCirleDraw\n";
    }
};
