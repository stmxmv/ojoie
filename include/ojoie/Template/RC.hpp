//
// Created by aojoie on 10/24/2022.
//

#ifndef OJOIE_RC_HPP
#define OJOIE_RC_HPP

#include "ojoie/Configuration/typedef.h"
#include <atomic>
#include <ojoie/Utility/Assert.h>

namespace AN {

template<typename Derived, template<typename> class _Dealloc = std::default_delete>
class RefCounted {
    std::atomic_int referenceCount;

public:
    RefCounted() : referenceCount(1) {}

    virtual ~RefCounted() = default;

    void retain() {
        ANAssert(referenceCount > 0);
        referenceCount.fetch_add(1, std::memory_order_acq_rel);
    }

    void release() {
        ANAssert(referenceCount > 0);
        if (referenceCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            _Dealloc<RefCounted>()(this);
        }
    }

    int getRetainCount() const {
        return referenceCount.load(std::memory_order_acquire);
    }

};

template<typename T>
class RefCountedPtr {
    T *_ptr;

public:

    template<typename>
    friend class RefCountedPtr;

    explicit RefCountedPtr(T *p = nullptr, bool retain = true) : _ptr(p) {
        if (_ptr && retain) _ptr->retain();
    }

    ~RefCountedPtr() {
        reset();
    }

    RefCountedPtr(RefCountedPtr const &rhs) : _ptr(rhs._ptr) {
        if (_ptr) _ptr->retain();
    }

    RefCountedPtr(RefCountedPtr &&other) noexcept : _ptr(other._ptr) {
        other.detach();
    }

    template<typename U>
        requires std::is_base_of_v<T, U>
    RefCountedPtr(RefCountedPtr<U> const &other) : _ptr(other._ptr) {
        if (_ptr) _ptr->retain();
    }

    template<typename U>
        requires std::is_base_of_v<T, U>
    explicit RefCountedPtr(RefCountedPtr<U> &&other) : _ptr(other._ptr) {
        other.detach();
    }

    RefCountedPtr &operator= (RefCountedPtr const &rhs) {
        if (&rhs != this) {
            RefCountedPtr(rhs).swap(*this);
        }
        return *this;
    }

    template<typename U>
        requires std::is_base_of_v<T, U>
    RefCountedPtr &operator= (RefCountedPtr<U> const &other) {
        if (&other != this) {
            RefCountedPtr(other).swap(*this);
        }
        return *this;
    }

    RefCountedPtr &operator= (RefCountedPtr &&other) noexcept {
        other.swap(*this);
        return *this;
    }

    template<typename U>
        requires std::is_base_of_v<T, U>
    RefCountedPtr &operator= (RefCountedPtr<U> &&other) {
        other.swap(*this);
        return *this;
    }

    RefCountedPtr &operator= (T *ptr) {
        RefCountedPtr(ptr).swap(*this);
        return *this;
    }

    RefCountedPtr &operator= (std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    void swap(RefCountedPtr &rhs) { std::swap(_ptr, rhs._ptr); }

    T *get() const { return _ptr; }

    T *detach() {
        T *ret = _ptr;
        _ptr   = nullptr;
        return ret;
    }

    void reset() {
        if (_ptr) {
            _ptr->release();
            _ptr = nullptr;
        }
    }

    T &operator* () const { return *_ptr; }

    T *operator->() const { return _ptr; }

    explicit operator bool () const { return _ptr; }

    bool operator== (std::nullptr_t) const {
        return _ptr == nullptr;
    }

    bool operator!= (std::nullptr_t) const {
        return !(*this == nullptr);
    }
};

template<typename T>
inline RefCountedPtr<T> ref_transfer(T *ptr) {
    return RefCountedPtr<T>(ptr, false);
}

}// namespace AN

#endif//OJOIE_RC_HPP
