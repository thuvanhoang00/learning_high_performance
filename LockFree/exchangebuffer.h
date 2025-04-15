#ifndef EXCHANGEBUFFER_H
#define EXCHANGEBUFFER_H
#include <optional>
#include <atomic>

template<typename T, uint32_t N, typename index_t = uint32_t>
class Storage;

template<uint32_t Size>
class IndexPool;

template <typename T, uint32_t C = 4>
class ExchangeBuffer
{
private:
    using storage_t     = Storage<T,C>;
    using indexpool_t   = IndexPool<C>;

    static constexpr index_t    NO_DATA = C;
    std::atomic<index_t>        m_index[NO_DATA];
    indexpool_t                 m_indices;
    storage_t                   m_storage;

public:
    // write value, replace existinggg value if any
    // return true if successful, false otherwise
    bool write(const T& value);

    // write value only if buffer is empty
    // return true if successful, false otherwise
    bool try_write(const T& value){;}
    
    // take value from buffer (buffer is empty afterwards)
    std::optional<T> take();

    // read value from buffer (value will still be in the buffer)
    std::optional<T> read(){}

    void free(index_t index){
        //call the destructor of T
        // return memory to storage
        m_storage.free(index);

        // return index to the pool
        m_indices.free(index);
    }
};

template <typename T, uint32_t C>
bool ExchangeBuffer<T, C>::write(const T& value){
#ifdef OLD_VER
    auto copy = new (std::nothrow) T(value); // not lock-free due to default memory allocator is not
    if(copy == nullptr){
        return false;
    }
    auto oldValue = m_value.exchange(copy);
    if(oldValue){
        delete oldValue; // not lock-free due to default memory allocator is not
    }
    return true;
#endif

    auto maybeIndex = m_indices.get();
    if(!maybeIndex) return false;

    auto index = maybeIndex.value();
    m_storage.store_at(index, value);

    auto oldIndex = m_index.exchange(index);
    if(oldIndex != NO_DATA){
        free(oldIndex);
    }
    return true;
}

template <typename T, uint32_t C>
std::optional<T> ExchangeBuffer<T, C>::take(){
#ifdef OLD_VER
    auto value = m_value.exchange(nullptr);

    if(value){
        auto ret = std::optional<T>(std::move(*value));
        delete value; // not lock-free due to default memory allocator is not
        return ret;
    }

    return std::nullopt;
#endif

}

template<typename T, uint32_t N, typename index_t = uint32_t>
class Storage{
public:
    void store_at(index_t index, const T& value);
    void free(index_t index);
    T* ptr(index_t index);
    T& operator[](index_t index);
};


template<uint32_t Size>
class IndexPool{
private:
    constexpr static uint8_t FREE=0;
    constexpr static uint8_t USED=1;
    std::atomic<uint8_t> m_slots[Size];
public:
    using index_t = uint32_t;

    IndexPool();

    std::optional<index_t> get();
    void free(index_t index);
};


#endif