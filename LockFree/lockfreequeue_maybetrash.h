#ifndef LOCKFREEQUEUE_H
#define LOCKFREEQUEUE_H
#include <atomic>
#include <cstddef>

namespace thu
{

template <typename T>
class LockFreeQueue
{
public:
    explicit LockFreeQueue(size_t capacity){
        _capacityMask = capacity-1;
        for(size_t i = 1; i <= sizeof(void*)*4; i <<=1){
            _capacityMask |= _capacityMask >> i;
        }
        _capacity = _capacityMask+1;

        _queue = (Node*)new char[sizeof(Node) * _capacity];
        for(size_t i = 0; i<_capacity; ++i){
            _queue[i].tail.store(i, std::memory_order_relaxed);
            _queue[i].head.store(-1, std::memory_order_relaxed);
        }
    
        _tail.store(0, std::memory_order_relaxed);
        _head.store(0, std::memory_order_relaxed);
    }

    ~LockFreeQueue(){
        for(size_t i = _head; i!= _tail; ++i){
            (&_queue[i & _capacityMask].data)->~T();
        }

        delete [] (char*)_queue;
    }

    size_t capacity() const {return _capacity;}
    size_t size() const
    {
        size_t head = _head.load(std::memory_order_acquire);
        return _tail.load(std::memory_order_relaxed) - head;
    }

    bool push(const T& data){
        Node* node;
        size_t tail = _tail.load(std::memory_order_relaxed);
        for(;;){
            node = &_queue[tail & _capacityMask];
            if(node->tail.load(std::memory_order_relaxed) != tail)
                return false;
            if(_tail.compare_exchange_weak(tail, tail+1, std::memory_order_relaxed))
                break;
        }

        new (&node->data)T(data);
        node->head.store(tail, std::memory_order_release);
        return true;
    }

    bool pop(T& result){
        Node* node;
        size_t head = _head.load(std::memory_order_relaxed);
        for(;;){
            node = &_queue[head & _capacityMask];
            if(node->head.load(std::memory_order_relaxed) != head)
                return false;
            if(_head.compare_exchange_weak(head, head+1, std::memory_order_relaxed))
                break;
        }
        result = node->data;
        (&node->data)->~T();
        node->tail.store(head + _capacity, std::memory_order_release);
        return true;
    }

    bool tryPush(T* data){
        size_t t = _tail.load(std::memory_order_relaxed);
        if(_head.load(std::memory_order_acquire) + _capacity == t) return false;
        _queue[t % _capacity].data = *data;
        _tail.store(t+1, std::memory_order_release);
        return true;
    }

    T* tryPop(){
        size_t h = _head.load(std::memory_order_relaxed);
        if(_tail.load(std::memory_order_acquire) == h) return nullptr;
        T* p = &_queue[h % _capacity].data;
        _head.store(h+1, std::memory_order_release);
        return p;
    }

private:
    struct Node{
        T data;
        alignas(64) std::atomic<size_t> tail;
        alignas(64) std::atomic<size_t> head;
    };

    size_t _capacityMask;
    Node* _queue;
    size_t _capacity;
    alignas(64) std::atomic<size_t> _tail;
    alignas(64) std::atomic<size_t> _head;
};


}


#endif