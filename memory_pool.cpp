#include <iostream>
#include <vector>
#include <memory>

template<typename T>
class MemoryPool{
public:
    explicit MemoryPool(size_t chunkSize = 1024)
        : chunkSize_(chunkSize){
            allocateChunk();
    }

    template<typename... Args>
    T* create(Args&&... args){
        if(freelist_.empty()){
            allocateChunk();
        }

        // take pointer from free list
        T* obj = freelist_.back();
        freelist_.pop_back();

        // placement new into allocated memory
        new(obj) T(std::forward<Args>(args)...); // construct an object of type T with perfect forwarding
        std::cout << "Create succesfully at: " << obj << std::endl;
        return obj;
    }

    void destroy(T* obj){
        if(obj){
            obj->~T();
            freelist_.push_back(obj);
        }
    }
    ~MemoryPool(){
        for(void* chunk : chunks_){
            ::operator delete(chunk);
        }
    }
private:
    void allocateChunk(){
        T* chunk = static_cast<T*>(::operator new(sizeof(T) * chunkSize_));
        chunks_.push_back(chunk);

        for(size_t i =0; i < chunkSize_; ++i){
            freelist_.push_back(&chunk[i]);
        }
    }

    size_t chunkSize_;
    std::vector<T*> freelist_; // available slots
    std::vector<void*> chunks_; // all allocated blocks
};

// Example usage
struct Point {
    int x, y;
    Point(int a=0, int b=0) : x(a), y(b) {
        std::cout << "Point(" << x << "," << y << ") constructed\n";
    }
    ~Point() {
        std::cout << "Point(" << x << "," << y << ") destroyed\n";
    }
};

int main(){
    MemoryPool<Point> pool(4);
    auto p1 = pool.create(1); // create Point(1-0)
    auto p2 = pool.create(1,1); // create Point(1-1)
    pool.destroy(p1);
    pool.destroy(p2);
    // memory reused from free list
    auto p3 = pool.create(2,2);
    pool.destroy(p3);
    return 0;
}