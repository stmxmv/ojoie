//
// Created by Aleudillonam on 7/29/2022.
//

#ifndef OJOIE_SPINLOCK_HPP
#define OJOIE_SPINLOCK_HPP

#include "ojoie/Configuration/typedef.h"
#include <atomic>

namespace AN {


class SpinLock {

    std::atomic_flag flag{};

public:
    __always_inline void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            cpu_relax();
        }
    }

    __always_inline void unlock() {
        flag.clear(std::memory_order_release);
    }
};


}

#endif//OJOIE_SPINLOCK_HPP
