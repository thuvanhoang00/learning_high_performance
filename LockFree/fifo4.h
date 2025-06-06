#ifndef FIFO4_H
#define FIFO4_H
#include <memory>
#include <cstddef>
#include <atomic>

template<typename T, typename Alloc = std::allocator<T>>
class Fifo4 : private Alloc
{
private:
    std::size_t _capacity;
    T* _ring;
    static_assert(std::atomic<std::size_t>::is_always_lock_free);

    alignas(64) std::atomic<std::size_t> _pushCursor{};
    // Exclusive to the push thread
    alignas(64) std::size_t _cachedPopCursor{};

    alignas(64) std::atomic<std::size_t> _popCursor{};
    // Exclusive to the pop thread
    alignas(64) std::size_t _cachedPushCursor{};

    auto full(std::size_t pushCursor, std::size_t popCursor) const {return (pushCursor-popCursor)==_capacity;}
    static auto empty(std::size_t pushCursor, std::size_t popCursor) {return pushCursor==popCursor;}
    auto element(std::size_t cursor){return &_ring[cursor % _capacity];}

public:
    explicit Fifo4(std::size_t capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}, _capacity{capacity}, _ring{std::allocator_traits<Alloc>::allocate(*this, capacity)}
        {}
    ~Fifo4(){
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
        if(full(pushCursor, _cachedPopCursor)){
            _cachedPopCursor = _popCursor.load(std::memory_order_acquire);
            if(full(pushCursor, _cachedPopCursor)){
                return false;
            }
        }
        new (&_ring[pushCursor % _capacity]) T(value);
        _pushCursor.store(pushCursor + 1, std::memory_order_release);
        return true;
    }

    auto pop(T& value){
        auto popCursor = _popCursor.load(std::memory_order_relaxed);
        if(empty(_cachedPushCursor, popCursor)){
            _cachedPushCursor = _pushCursor.load(std::memory_order_acquire);
            if(empty(_cachedPushCursor, popCursor)){
                return false;
            }
        }

        value = _ring[popCursor % _capacity];
        _ring[popCursor % _capacity].~T();
        _popCursor.store(popCursor+1, std::memory_order_release);
        return true;
    }
};


#endif