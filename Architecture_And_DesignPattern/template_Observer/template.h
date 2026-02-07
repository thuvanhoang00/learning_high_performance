#pragma once
#include <thread>
#include <set>
#include <random>
#include <iostream>
template<typename Data>
class Subscriber{
public:
    virtual ~Subscriber() = default;
    virtual void on_data(const Data&) = 0;
};


template<typename Data>
class Publisher{
protected:
    std::set<Subscriber<Data>*> subs_;
public:
    virtual ~Publisher() = default;
    void attach(Subscriber<Data>* sub){
        subs_.insert(sub);
    }

    void publish(const Data& data){
        for(auto sub : subs_){
            sub->on_data(data);
        }
    }
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

class MultiObjectHandlder : public Publisher<MultiObjOutput>{
private:
    std::thread worker_thread_;
public:
    MultiObjectHandlder(){
        worker_thread_ = std::thread(&MultiObjectHandlder::do_work, this);
    }
    ~MultiObjectHandlder(){
        if(worker_thread_.joinable()){
            worker_thread_.join();
        }
    }

    void do_work(){
        while(true){
        static int seq=0;
        int data = rand()%1000;
        std::cout << "[MultiObj] produce:" << data << std::endl;
        publish(MultiObjOutput{seq, data}); 
        ++seq;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

class HumanPoint : public Subscriber<MultiObjOutput>, public Publisher<HumanPointOutput>{
private:
    std::thread worker_thread_;
    MultiObjOutput out_put_;
public:
    HumanPoint(){
        worker_thread_ = std::thread(&HumanPoint::do_work, this);
    }
    ~HumanPoint(){
        if(worker_thread_.joinable()){
            worker_thread_.join();
        }
    }

    void do_work(){
        while(true){
        static int seq = 0;
        auto data = out_put_.data_ + 1;
        std::cout << "[HumanPoint] produce: " << data << std::endl;
        publish(HumanPointOutput{seq, data});
        ++seq;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void on_data(const MultiObjOutput& data) override{
        out_put_ = data;
    }
};

class Seatbelt : public Subscriber<MultiObjOutput>, public Subscriber<HumanPointOutput> {
private:
    std::thread worker_thread_;
    MultiObjOutput multi_output_;
    HumanPointOutput human_output_;
public:
    Seatbelt(){
        worker_thread_ = std::thread(&Seatbelt::do_work, this);
    }
    ~Seatbelt(){
        if(worker_thread_.joinable()){
            worker_thread_.join();
        }
    }

    void on_data(const MultiObjOutput& data) override{
        multi_output_ = data;        
    }

    void on_data(const HumanPointOutput& data) override{
        human_output_ = data;
    }

    void do_work(){
        while(true){

        if(human_output_.seq_ == multi_output_.seq_){
            std::cout << "Seatbelt processing on: ";
            std::cout << "[Seatbelt] multidata seq:" << multi_output_.seq_ << "-data:" << multi_output_.data_ 
                << " | humandata seq:" << human_output_.seq_ << "-data:" << human_output_.data_ << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }
};