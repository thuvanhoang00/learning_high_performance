#ifndef FIFO1_H
#define FIFO1_H
#include <memory>
#include <cstddef>

template<typename T, typename Alloc = std::allocator<T>>
class Fifo1 : private Alloc
{
private:
    std::size_t _capacity;
    T* _ring;
    std::size_t _pushCursor{};
    std::size_t _popCursor{};
public:
    explicit Fifo1(std::size_t capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}, _capacity{capacity}, _ring{std::allocator_traits<Alloc>::allocate(*this, capacity)}
        {}
    ~Fifo1(){
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