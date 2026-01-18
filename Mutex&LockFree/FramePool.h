#include <memory>
#include <mutex>
#include <condition_variable>
#include <stack>
#include <vector>

template<typename T>
class FramePool{
private:
    void release(T* frame){
        std::lock_guard<std::mutex> lk(mtx_);
        pool_.push(frame);
        cv_.notify_one();
    }

    std::stack<T*> pool_; // cache warm 
    std::mutex mtx_;
    std::condition_variable cv_;
public:
    FramePool(size_t poolSize){
        for(size_t i = 0; i< poolSize; ++i){
            auto frame = new T();
            pool_.push(frame);
        }
    }

    ~FramePool(){
        while(!pool_.empty()){
            delete pool_.front();
            pool_.pop();
        }
    }

    std::shared_ptr<T> acquire(){
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [this](){
            return !pool_.empty();
        });
        T* rawPtr = pool_.front();
        pool_.pop();

        // When reference count=0, call this lambda deleter
        // To release ptr to pool
        return std::shared_ptr<T>(rawPtr, [this](T* frame){
            this->release(frame);
        });
    }
};

/*
// Create the Pool (Holds the 10MB chunks)
FramePool pool(10, 1080, 1920, CV_8UC3);

// Create 5 Queues. 
// Size 1 is enough for ADAS (we only want the absolute latest frame).
// This allocates the internal vectors NOW, not later.
std::vector<RingBuffer<std::shared_ptr<cv::Mat>>*> queues;
RingBuffer<std::shared_ptr<cv::Mat>> q1(1), q2(1), q3(1), q4(1), q5(1);
queues = {&q1, &q2, &q3, &q4, &q5};
*/