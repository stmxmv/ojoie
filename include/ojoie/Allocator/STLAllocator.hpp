//
// Created by aojoie on 6/23/2023.
//

#pragma once

#include <ojoie/Allocator/MemoryDefines.h>
#include <cstddef>

namespace AN {

template<typename T, int align = kDefaultMemoryAlignment>
class STLAllocator {
public:
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;
    typedef T*        pointer;
    typedef const T*  const_pointer;
    typedef T&        reference;
    typedef const T&  const_reference;
    typedef T         value_type;

    STLAllocator() {}

    template <typename U, int _align>
    STLAllocator(const STLAllocator<U, _align>& alloc) noexcept {}

    template<typename U>
    struct rebind {
        typedef STLAllocator<U, align> other;
    };

    pointer address (reference x) const { return &x; }
    const_pointer address (const_reference x) const { return &x; }

    pointer allocate (size_type count, void const* /*hint*/ = 0) {
        return (pointer)AN_MALLOC_ALIGNED(count * sizeof(T), align);
    }

    void deallocate (pointer p, size_type /*n*/) {
        AN_FREE(p);
    }

    size_type max_size() const noexcept { return 0x7fffffff; }

    void construct(pointer p, const T& val) { new (p) T(val); }

    void destroy(pointer p) { p->~T(); }
};

}
