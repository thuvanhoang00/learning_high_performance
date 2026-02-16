#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <iostream>
#include <mutex>
#include <memory>
#include <algorithm>
class EventBus;

class SubscriptionHandle{
private:
    EventBus* bus_;
    int id_ = -1;
public:
    SubscriptionHandle(EventBus* bus, int id) : bus_(bus), id_(id) {}
    SubscriptionHandle(const SubscriptionHandle&) = delete;
    SubscriptionHandle& operator=(const SubscriptionHandle&) = delete;

    SubscriptionHandle(SubscriptionHandle&& other) noexcept
        : bus_(other.bus_), id_(other.id_)
    {
        other.bus_ = nullptr;
        id_ = -1;
    }

    SubscriptionHandle& operator=(SubscriptionHandle&& other) noexcept{
        if(this != &other){
            bus_ = other.bus_;
            id_ = other.id_;
            other.bus_ = nullptr;
            other.id_ = -1;
        }

        return *this;
    }
    ~SubscriptionHandle();
};


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
    std::unordered_map<std::type_index, int> type_subscribers_;

    std::mutex mtx_;
    int id_ = -1;
public:
    template<typename T>
    SubscriptionHandle subscribe(std::function<void(const T&)> callback){
        
        std::lock_guard<std::mutex> lk(mtx_);
        ++id_;
        type_subscribers_[typeid(T)] = id_;

        subscribers_[typeid(T)].push_back(
            [callback](const void* data){
            callback(*static_cast<const T*>(data));
        }
        );

        return SubscriptionHandle(this, id_);
    }

    template<typename T>
    void publish(const T& message){
        std::unordered_map<std::type_index, std::vector<InternalCallback>> snapshot;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            snapshot = subscribers_; // FIX re-entrancy problem
        }
        auto it = snapshot.find(typeid(T));
        if(it != snapshot.end()){
            for(auto& slot : it->second){
                slot(&message);
            }
        }
    }

    void unsubscribe(int id){
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = type_subscribers_.begin();
        for( ; it != type_subscribers_.end(); ++it){
            if(it->second == id)
                break;
        }
        if(it == type_subscribers_.end()){
            return ;
        }

        auto sub_it = subscribers_.find(it->first);

        if(sub_it != subscribers_.end()){
            subscribers_.erase(sub_it);
        }

        type_subscribers_.erase(it);

    }
};

SubscriptionHandle::~SubscriptionHandle()
{
    if(bus_ && id_ != -1)
        bus_->unsubscribe(id_);
}
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
    std::unique_ptr<SubscriptionHandle> multi_obj_sub_handle_;
public:
    HumanPoint(EventBus& bus) : bus_(bus){
        multi_obj_sub_handle_ = std::make_unique<SubscriptionHandle>(bus_.subscribe<MultiObjOutput>([this](const MultiObjOutput& data){
            processInput(data);
        }));
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
    std::unique_ptr<SubscriptionHandle> multi_obj_sub_handle_;
    std::unique_ptr<SubscriptionHandle> human_sub_handle_;

public:
    Seatbelt(EventBus& bus) : bus_(bus){
        multi_obj_sub_handle_ = std::make_unique<SubscriptionHandle>(bus_.subscribe<MultiObjOutput>([this](const MultiObjOutput& data){
            processInput(data);
        }));
        human_sub_handle_ = std::make_unique<SubscriptionHandle>(bus_.subscribe<HumanPointOutput>([this](const HumanPointOutput& data){
            processInput(data);
        }));
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