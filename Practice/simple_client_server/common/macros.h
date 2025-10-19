#ifndef MACROS_H
#define MACROS_H

#include <cstring>
#include <iostream>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

inline auto ASSERT(bool cond, const std::string& msg) noexcept
{
    if(UNLIKELY(!cond)){
        std::cerr << msg << std::endl;
        exit(EXIT_FAILURE);
    }
}

inline auto FATAL(const std::string& msg) noexcept
{
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

#endif