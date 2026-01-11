#pragma once
#include <iostream>
#include "shape.h"

template<typename DrawStrategy>
class Circle : public Shape{
private:
    DrawStrategy draw_strategy_;
public:
    Circle(DrawStrategy strategy) : draw_strategy_(std::move(strategy)) {}
    void draw() override{
        std::cout << "Drawing circle: ";
        draw_strategy_.draw();
    }
};

