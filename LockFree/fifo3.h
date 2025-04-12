#ifndef FIFO3_H
#define FIFO3_H
#include <memory>
#include <cstddef>
#include <atomic>

template<typename T, typename Alloc = std::allocator<T>>
class Fifo3 : private Alloc
{
private:
    std::size_t _capacity;
    T* _ring;
    static_assert(std::atomic<std::size_t>::is_always_lock_free);

    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> _pushCursor{};
    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> _popCursor{};

    // Padding to avoid false sharing with adjacent objects
    char _padding[std::hardware_destructive_interference_size - sizeof(std::size_t)];

    auto full(std::size_t pushCursor, std::size_t popCursor) const {return (pushCursor-popCursor)==_capacity;}
    static auto empty(std::size_t pushCursor, std::size_t popCursor) {return pushCursor==popCursor;}
    auto element(std::size_t cursor){return &_ring[cursor % _capacity];}

public:
    explicit Fifo3(std::size_t capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}, _capacity{capacity}, _ring{std::allocator_traits<Alloc>::allocate(*this, capacity)}
        {}
    ~Fifo3(){
        while(not empty()){
            _ring[_popCursor % _capacity].~T();
            ++_popCursor;
        }
        std::allocator_traits<Alloc>::deallocate(*this, _ring, _capacity);
    }

    auto capacity() const {return _capacity;}
    auto size() const {return _pushCursor - _popCursor;}
    auto empty() const {return size() == 0;}
    auto full() const {return size() ==  _capacity;}

    auto push(T const& value){
        auto pushCursor = _pushCursor.load(std::memory_order_relaxed);
        auto popCursor = _popCursor.load(std::memory_order_acquire);
        if(full(pushCursor, popCursor)){
            return false;
        }
        new (element(pushCursor)) T(value);
        _pushCursor.store(pushCursor+1, std::memory_order_release);
        return true;
    }

    auto pop(T& value){
        auto pushCursor = _pushCursor.load(std::memory_order_acquire);
        auto popCursor = _popCursor.load(std::memory_order_relaxed);
        if(empty(pushCursor, popCursor)){
            return false;
        }
        value = *element(popCursor);
        element(popCursor)->~T();
        _popCursor.store(popCursor+1, std::memory_order_release);
        return true;
    }
};


#endif