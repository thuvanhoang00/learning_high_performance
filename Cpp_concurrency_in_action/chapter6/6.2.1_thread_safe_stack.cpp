#include <exception>
#include <stack>
#include <mutex>
#include <memory>
#include <thread>
#include <iostream>
struct empty_stack : std::exception{
    const char* what() const throw() override{
        return "empty stack";
    }
};

template<typename T>
class threadsafe_stack{
private:
    std::stack<T> data_;
    mutable std::mutex m_;
public:
    threadsafe_stack(){}
    threadsafe_stack(const threadsafe_stack& other)
    {
        std::lock_guard<std::mutex> lock(other.m_);
        data_ = other.data_;
    }
    
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;
    
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m_);
        data_.push(std::move(new_value));
    }

    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock(m_);
        if(data_.empty()) throw empty_stack();
        std::shared_ptr<T> const res{
            std::make_shared<T>(std::move(data_.top()))
        };
        data_.pop();
        return res;
    }
    
    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m_);
        if(data_.empty()) throw empty_stack();
        value = std::move(data_.top());
        data_.pop();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_);
        return data_.empty();
    }

    int size() const
    {
        return data_.size();
    }
};

int main()
{
    threadsafe_stack<int> tsstack;
    tsstack.push(1);
    auto push_func = [&](){
        for(int i=0; i<100; ++i){
            tsstack.push(i);
        }
    };
    auto pop_func = [&](){
        for(int i=0; i<100; ++i){
            int temp;
            tsstack.pop(temp);
        }
    };

    std::thread t1(push_func);
    std::thread t2(pop_func);
    t1.join();
    t2.join();

    std::cout << tsstack.size() << std::endl;

    return 0;
}