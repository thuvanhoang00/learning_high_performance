#pragma once
class Shape{
public:
    virtual ~Shape() = default;
    virtual void draw() = 0;
};