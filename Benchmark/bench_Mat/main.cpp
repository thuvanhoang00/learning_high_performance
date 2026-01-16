#include <benchmark/benchmark.h>
#include <vector>
#include <array>
#include <thread>
#include <iostream>
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
    Point(int x, int y) : x(x), y(y) {}
    Point(const Point& other){
        x=other.x;
        y=other.y;
    }

    Point& operator=(const Point&) = default;

    Point(Point&& other) noexcept{
        x = std::move(other.x);
        y = std::move(other.y);
    }
    Point& operator=(Point&& other) noexcept{
        if(this != &other){
            x=std::move(other.x);
            y=std::move(other.y);
        }
        return *this;
    }
};

constexpr int N = 1440*1080*3;

std::vector<Point> src_;


struct Mat{
    int sequence_id = 0;
    std::vector<Point> data_;
public:
    Mat(){}
    Mat(size_t n) : data_(n, {0,0}) {}

    Mat(const Mat& other){
        data_ = other.data_;
    }
    Mat& operator=(const Mat& other){
        if(this != &other){
            data_ = other.data_;
        }
        return *this;
    }

    Mat(Mat&& other) noexcept{
        data_ = std::move(other.data_);
    }
    Mat& operator=(Mat&& other) noexcept{
        if(this != &other){
            data_ = std::move(other.data_);
        }
        return *this;
    }
};

void init_src(){
    src_.reserve(N);
    for(int i=0; i<N; ++i){
        src_[i] = Point{i, i+1};
    }
}

void bench_copy(benchmark::State& s){
    init_src();
    // const size_t Size = s.range();
    SPSC_LFQueue<Mat, 100> lfq;

    auto writer = [&](){
        Mat temp(N);
        for(int i=0; i<100; ++i){
            temp.sequence_id=i;
            benchmark::DoNotOptimize(std::copy(src_.begin(), src_.end(), temp.data_.begin()));        
            while(!lfq.push(temp)){
                temp.sequence_id=i;
                benchmark::DoNotOptimize(std::copy(src_.begin(), src_.end(), temp.data_.begin()));  
            }
        }
    };

    auto reader = [&](){
        int count=0;
        Mat temp(N);
        while(count<100){
            if(lfq.pop(temp)){
                std::cout << "[Copy] popped value: " << temp.sequence_id << std::endl;
                count++;
            }
        }
    };

    for(auto _ : s){
        std::jthread t1(writer);
        std::jthread t2(reader);
    }
}

void bench_move(benchmark::State& s){
    init_src();
    // const size_t Size = s.range();
    SPSC_LFQueue<Mat, 100> lfq;

    auto writer = [&](){
        Mat temp(N);
        for(int i=0; i<100; ++i){
            temp.sequence_id = i;
            benchmark::DoNotOptimize(std::copy(src_.begin(), src_.end(), temp.data_.begin()));        
            while(!lfq.push(temp)){
                temp.sequence_id=i;
                benchmark::DoNotOptimize(std::copy(src_.begin(), src_.end(), temp.data_.begin()));  
            }
        }
    };

    auto reader = [&](){
        int count=0;
        Mat temp(N);
        while(count<100){
            if(lfq.pop(temp)){
                std::cout << "[Move] popped value: " << temp.sequence_id << std::endl;
                count++;
            }
        }
    };

    for(auto _ : s){
        std::jthread t1(writer);
        std::jthread t2(reader);
    }
}

BENCHMARK(bench_copy)->DenseRange(1, 5);
BENCHMARK(bench_move)->DenseRange(1, 5);

BENCHMARK_MAIN();
