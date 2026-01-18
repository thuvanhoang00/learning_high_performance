#ifndef FIFO5_H
#define FIFO5_H
#include <memory>
#include <cstddef>
#include <atomic>

template <typename T, typename Alloc = std::allocator<T>>
class Fifo5 : private Alloc
{
private:
    static_assert(std::is_trivial_v<T>, "Fifo5 requires a trivial type T");
    std::size_t _capacity;
    T *_ring;
    static_assert(std::atomic<std::size_t>::is_always_lock_free);

    alignas(64) std::atomic<std::size_t> _pushCursor{};
    // Exclusive to the push thread
    alignas(64) std::size_t _cachedPopCursor{};

    alignas(64) std::atomic<std::size_t> _popCursor{};
    // Exclusive to the pop thread
    alignas(64) std::size_t _cachedPushCursor{};

    auto full(std::size_t pushCursor, std::size_t popCursor) const { return (pushCursor - popCursor) == _capacity; }
    static auto empty(std::size_t pushCursor, std::size_t popCursor) { return pushCursor == popCursor; }
    auto element(std::size_t cursor) { return &_ring[cursor % _capacity]; }

public:
    explicit Fifo5(std::size_t capacity, Alloc const &alloc = Alloc{})
        : Alloc{alloc}, _capacity{capacity}, _ring{std::allocator_traits<Alloc>::allocate(*this, capacity)}
    {
    }
    ~Fifo5()
    {
        while (not empty())
        {
            _ring[_popCursor % _capacity].~T();
            ++_popCursor;
        }
        std::allocator_traits<Alloc>::deallocate(*this, _ring, _capacity);
    }

    auto capacity() const { return _capacity; }
    auto size() const { return _pushCursor - _popCursor; }
    auto empty() const { return size() == 0; }
    auto full() const { return size() == _capacity; }

    // pusher_t class
    class pusher_t
    {
    private:
        Fifo5* _fifo{};
        std::size_t _cursor;
    public:
        pusher_t() = default;
        explicit pusher_t(Fifo5* fifo, std::size_t cursor) : _fifo(fifo), _cursor(cursor){}

        // Not copyable; is moveable
        ~pusher_t(){
            if(_fifo){
                _fifo->_pushCursor.store(_cursor+1, std::memory_order_release);
            }
        }
        explicit operator bool() const {return _fifo;}
        void release(){_fifo = {};}
        T* get() {return _fifo->element(_cursor);}
        T* operator->(){return get();}
        T const* operator->() const{return get();}
        T& operator*(){return *get();}
    };
    pusher_t push(){
        auto pushCursor = _pushCursor.load(std::memory_order_relaxed);
        if(full(pushCursor, _cachedPopCursor)){
            _cachedPopCursor = _popCursor.load(std::memory_order_acquire);
            if(full(pushCursor, _cachedPopCursor)){
                return pusher_t{};
            }
        }
        return pusher_t{this, pushCursor};
    }
    bool push(T const &value){
        auto pusher = push();
        if(pusher){
            *pusher = value;
            return true;
        }
        return false;
    }

    // popper_t class
    class popper_t
    {
    private:
        Fifo5 *_fifo{};
        std::size_t _cursor;

    public:
        popper_t() = default;
        explicit popper_t(Fifo5 *fifo, std::size_t cursor) : _fifo(fifo), _cursor(cursor) {}

        // Not copyable; is moveable
        ~popper_t()
        {
            if (_fifo)
            {
                _fifo->_pushCursor.store(_cursor + 1, std::memory_order_release);
            }
        }
        explicit operator bool() const { return _fifo; }
        void release() { _fifo = {}; }
        T *get() { return _fifo->element(_cursor); }
        T *operator->() { return get(); }
        T const *operator->() const { return get(); }
        T& operator*(){return *get();}
    };
    popper_t pop(){
        auto popCursor = _popCursor.load(std::memory_order_relaxed);
        if(empty(_cachedPushCursor, popCursor)){
            _cachedPushCursor = _pushCursor.load(std::memory_order_acquire);
            if(empty(_cachedPushCursor, popCursor)){
                return popper_t{};
            }
        }
        return popper_t{this, popCursor};
    }
    bool pop(T& value){
        auto popper = pop();
        if(popper){
            value = *popper;
            return true;
        }
        return false;
    }
};



#endif