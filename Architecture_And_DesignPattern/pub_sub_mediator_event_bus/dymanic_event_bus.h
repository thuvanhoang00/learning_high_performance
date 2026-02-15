#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <iostream>
#include <mutex>
#include <memory>
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

    std::mutex mtx_;
    std::vector<std::weak_ptr<InternalCallback>> callbacks_;
public:
    template<typename T>
    std::shared_ptr<void> subscribe(std::function<void(const T&)> callback){
        auto wrapped = [cb = std::move(callback)](const void* data){
            cb(*static_cast<const T*>(data));
        };
        std::lock_guard<std::mutex> lk(mtx_);
        auto handle = std::make_shared<InternalCallback>(std::move(wrapped));

        // subscribers_[typeid(T)].push_back([callback](const void* data){
        //     callback(*static_cast<const T*>(data));
        // });
        subscribers_[typeid(T)].push_back(*handle);

        return handle;
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
    std::shared_ptr<void> multiObjSub_;
public:
    HumanPoint(EventBus& bus) : bus_(bus){
        multiObjSub_ = bus_.subscribe<MultiObjOutput>([this](const MultiObjOutput& data){
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
    std::shared_ptr<void> multiObjSub_, humanOutputSub_;
public:
    Seatbelt(EventBus& bus) : bus_(bus){
        multiObjSub_ = bus_.subscribe<MultiObjOutput>([this](const MultiObjOutput& data){
            processInput(data);
        });
        humanOutputSub_ = bus_.subscribe<HumanPointOutput>([this](const HumanPointOutput& data){
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