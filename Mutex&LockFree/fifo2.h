#ifndef FIFO2_H
#define FIFO2_H
#include <memory>
#include <cstddef>
#include <atomic>

template<typename T, typename Alloc = std::allocator<T>>
class Fifo2 : private Alloc
{
private:
    std::size_t _capacity;
    T* _ring;
    std::atomic<std::size_t> _pushCursor{};
    std::atomic<std::size_t> _popCursor{};

    static_assert(std::atomic<std::size_t>::is_always_lock_free);
public:
    explicit Fifo2(std::size_t capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}, _capacity{capacity}, _ring{std::allocator_traits<Alloc>::allocate(*this, capacity)}
        {}
    ~Fifo2(){
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
        if(full()) return false;

        new (&_ring[_pushCursor % _capacity]) T(value);
        ++_pushCursor;
        return true;
    }

    auto pop(T& value){
        if(empty()) return false;

        value = _ring[_popCursor % _capacity];
        _ring[_popCursor % _capacity].~T();
        ++_popCursor;
        return true;
    }
};


#endif