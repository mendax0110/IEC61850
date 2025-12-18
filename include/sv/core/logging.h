#pragma once

#include <iostream>
#include <cassert>

#define ASSERT(expr, msg)                                                                                   \
    do                                                                                                      \
    {                                                                                                       \
        if (!(expr))                                                                                        \
        {                                                                                                   \
            std::cerr << "Assertion failed: " << msg << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            assert(false);                                                                                  \
        }                                                                                                   \
    } while (0)

#define LOG_ERROR(msg) \
    std::cerr << "Error: " << msg << " at " << __FILE__ << ":" << __LINE__ << std::endl

#define LOG_INFO(msg) std::cout << "Info: " << msg << std::endl