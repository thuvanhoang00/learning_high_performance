#include <memory>
#include <vector>
#include <iostream>
namespace thu{
template<typename T, typename Allocator = std::allocator<T>>
class vector{
private:
    T* begin_ = nullptr;
    T* end_ = nullptr;
    T* capacity_end_ = nullptr;
    Allocator alloc_; // allocator instance (uses EBO for stateless allocators)

public:
    vector() noexcept = default;
    explicit vector(const Allocator& alloc) noexcept : alloc_(alloc) {}
    explicit vector(size_t count, const T& value = T(), const Allocator& alloc = Allocator())
    : alloc_(alloc)
    {
        if(count > 0){
            begin_ = alloc_.allocate(count);
            capacity_end_ = begin_ + count;
            end_ = begin_;
        }
        try
        {
            for(; count > 0; --count, ++end_){
                alloc_.construct(end_, value); // copy-construct each element
            }
        }
        catch(...)
        {
            // cleanup if construction fails
            while(end_ != begin_){
                alloc_.destroy(--end_);
            }
            alloc_.deallocate(begin_, capacity());
            throw;
        }        
    }
    vector(const vector& other)
        : alloc_(other.alloc_)
    {
        if(other.size() > 0){
            begin_ = alloc_.allocate(other.capacity());
            capacity_end_ = begin_ + other.capacity();
            end_ = begin_;

            try
            {
                for(T* src = other.begin_; src != other.end_; ++src, ++end_){
                    alloc_.construct(end_, *src); // copy each element
                }
            }
            catch(...)
            {
                while(end_ != begin_){
                    alloc_.destroy(--end_);
                }
                alloc_.deallocate(begin_, other.capacity());
                throw;
            }            
        }
    }
    vector(vector&& other) noexcept
    : begin_(other.begin_), end_(other.end_), capacity_end_(other.capacity_end_), alloc_(std::move(other.alloc_))
    {
        other.begin_ = other.end_ = other.capacity_end_ = nullptr;
    }

    ~vector(){
        for(T* p = begin_; p != end_; ++p){
            alloc_.destroy(p);
        }
        if(begin_){
            alloc_.deallocate(begin_, capacity());
        }
    }

    size_t size() const noexcept {return end_ - begin_;}
    size_t capacity() const noexcept {return capacity_end_ - begin_;}
    T* data() const noexcept {return begin_;}
    void push_back(const T& value){
        if(end_ == capacity_end_){
            reserve(capacity() == 0 ? 1 : capacity()*2);
        }
        alloc_.construct(end_++, value);
    }
    void reserve(size_t new_cap){
        if(new_cap <= capacity()) return;

        T* new_block = alloc_.allocate(new_cap);
        T* new_end = new_block;
        for(T* src = begin_; src != end_; ++src, ++new_end){
            alloc_.construct(new_end, std::move(*src));
            alloc_.destroy(src);
        }
        alloc_.deallocate(begin_, capacity());
        begin_ = new_block;
        end_ = new_end;
        capacity_end_ = begin_ + new_cap;
    }

    T& operator[](int i){
        if(i<0 || i >= size()) throw;
        return *(begin_ + i);
    }

    T& operator[](int i) const {
        if(i<0 || i >= size()) throw;
        return *(begin_ + i);
    }
};
}

int test_thuvector(const thu::vector<int>& vec)
{
    long long ret{};
    for(int i=0; i<vec.size(); ++i){
        ret += vec[i];
    }
    return ret;
}

int test_stdvector(const std::vector<int>& vec)
{
    long long ret{};
    for(int i=0; i<vec.size(); ++i){
        ret += vec[i];
    }
    return ret;
}
int main(){
    const int N = 1<<20;
    thu::vector<int> thuvec(N);
    std::vector<int> stdvec(N);
    for(int i=0; i < N; ++i){
        thuvec.push_back(1);
        stdvec.push_back(1);
    }
    std::cout << test_thuvector(thuvec) << std::endl;
    std::cout << test_stdvector(stdvec) << std::endl;
    return 0;
}