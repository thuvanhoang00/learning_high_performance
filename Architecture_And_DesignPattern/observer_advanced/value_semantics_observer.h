#pragma once
#include <functional>
template<typename Subject, typename StateTag>
class Observer{
using OnUpdate = std::function<void(const Subject&, StateTag)>;
private:
    OnUpdate onUpdate_;
public:
    Observer(OnUpdate onUpdate) : onUpdate_(std::move(onUpdate)) {}

    void update(const Subject& subject, StateTag tag){
        onUpdate_(subject, tag);
    }
};