#pragma once
#include <vector>
#include <cstddef>
template<typename Derived>
class DenseVector{
protected:
    ~DenseVector() = default; // prevent compiler gen move. Core guideline C.35 either public and virtuual or protected
public:
    Derived& derived(){
        return static_cast<Derived&>(*this);
    }
    const Derived& derived() const {
        return static_cast<const Derived&>(*this);
    }

    size_t size() const {
        return derived().size();
    }
};

template<typename T>
class DynamicVector : public DenseVector<DynamicVector<T>>
{
private:
    std::vector<T> vec_;
public:
    DynamicVector(size_t sz) : vec_(sz) {}
    size_t size() const{
        return vec_.size();
    }
};
