//
// Created by aojoie on 10/24/2022.
//

#ifndef OJOIE_RC_HPP
#define OJOIE_RC_HPP

#include <ojoie/Core/typedef.h>
#include <atomic>

namespace AN {

template<typename Derived>
class ReferenceCounted {
    std::atomic_int referenceCount;
public:

    ReferenceCounted() : referenceCount(1) {}

    void retain() {
        ANAssert(referenceCount > 0);
        referenceCount.fetch_add(1, std::memory_order_acq_rel);
    }

    void release() {
        ANAssert(referenceCount > 0);
        if (referenceCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete (Derived *)this;
        }
    }

};

}

#endif//OJOIE_RC_HPP
