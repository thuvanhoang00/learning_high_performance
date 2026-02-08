#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <iostream>
struct MultiObjOutput{
    int seq_;
    int data_;
};

struct HumanPointOutput{
    int seq_;
    int data_;
};

struct SeatbeltOutput{
    int seq_;
    int data_;
};

class EventBus{
private:
    using InternalCallback = std::function<void(const void*)>;
    std::unordered_map<std::type_index, std::vector<InternalCallback>> subscribers_;
public:
    template<typename T>
    void subscribe(std::function<void(const T&)> callback){
        subscribers_[typeid(T)].push_back([callback](const void* data){
            callback(*static_cast<const T*>(data));
        });
    }

    template<typename T>
    void publish(const T& message){
        auto it = subscribers_.find(typeid(T));
        if(it != subscribers_.end()){
            for(auto& slot : it->second){
                slot(&message);
            }
        }
    }
};

class MultiObjectHandler{
private:
    EventBus& bus_;
public:
    MultiObjectHandler(EventBus& bus) : bus_(bus){}
    void produce(){
        bus_.publish<MultiObjOutput>(MultiObjOutput{1,1});
    }
};

class HumanPoint{
private:
    EventBus& bus_;
public:
    HumanPoint(EventBus& bus) : bus_(bus){
        bus_.subscribe<MultiObjOutput>([this](const MultiObjOutput& data){
            processInput(data);
        });
    }

    void processInput(const MultiObjOutput& data){
        std::cout << "HumanPoint Received: " << data.data_ << std::endl;
        std::cout << "HumanPoint produce: " << data.data_+1 << std::endl;
        bus_.publish<HumanPointOutput>(HumanPointOutput{1, data.data_+1});
    }
};


class Seatbelt{
private:
    EventBus& bus_;

public:
    Seatbelt(EventBus& bus) : bus_(bus){
        bus_.subscribe<MultiObjOutput>([this](const MultiObjOutput& data){
            processInput(data);
        });
        bus_.subscribe<HumanPointOutput>([this](const HumanPointOutput& data){
            processInput(data);
        });
    }

    void processInput(const MultiObjOutput& data){
        std::cout << "Seatbelt received multiobj: " << data.data_ << std::endl;
        do_work();
    }

    void processInput(const HumanPointOutput& data){
        std::cout << "Seatbelt received humanpoint: " << data.data_ << std::endl;
        do_work();
    }

    void do_work(){
        std::cout << "Seatbelt's dowork" << std::endl;
    }
};