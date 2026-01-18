#include <array>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
template<typename T, size_t N>
class SPSC_LFQueue{
private:
    std::array<T, N> buffer_{};
    std::atomic<size_t> size_{0};
    size_t read_pos_{0};
    size_t write_pos_{0};

    bool do_push(auto&& t){
        // full
        if(size_.load() == N){
            return false;
        }
        buffer_[write_pos_] = std::forward<decltype(t)>(t);
        write_pos_ = (write_pos_ + 1) % N;
        size_.fetch_add(1);
        return true;
    }

public:
    bool push(T&& t){ return do_push(std::move(t));}
    bool push(const T& t){ return do_push(t);}

    bool pop(T& t){
        if(size_.load() > 0){
            t = std::move(buffer_[read_pos_]);
            read_pos_ = (read_pos_ + 1) % N;   
            size_.fetch_sub(1);
            return true;
        }
        return false;
    }

    size_t size() const noexcept { return size_.load();}
};

struct Point{
    int x;
    int y;
};

constexpr int N = 1440*1080*3;

struct Mat{
private:
    std::vector<Point> data_;
public:
    Mat() : data_(N, {1,2}) {}
    int sequence_id = 0;
};

SPSC_LFQueue<Mat, 10> large_lfq;

int main(){

    auto writer = [](){
        for(int i=0; i<1024; ++i){
            Mat temp;
            temp.sequence_id=i;
            while(!large_lfq.push(temp));
        }
    };

    auto reader = [](){
        Mat popped_value;
        int count=0;
        while (count < 1024)
        {
            if(large_lfq.pop(popped_value)){
                ++count;
                std::cout << "popped Mat.sequence_id: " << popped_value.sequence_id << std::endl;
            }
        }

    };

    std::thread t1(writer);
    std::thread t2(reader);

    t1.join();
    t2.join();
    return 0;
}