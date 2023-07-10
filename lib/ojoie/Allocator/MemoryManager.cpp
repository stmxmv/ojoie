//
// Created by aojoie on 4/12/2023.
//

#include "Allocator/MemoryManager.h"
#include "Allocator/MemoryDefines.h"
#include <cstdlib>
#include <cstring>

#ifdef WITH_TBB
#include <tbb/scalable_allocator.h>
#endif//WITH_TBB

#ifdef __UNIX__
static void *aligned_realloc_unix(void *ptr, size_t align, size_t size) {
    if ((size == 0) || (align <= alignof(max_align_t)))
    {
        return realloc(ptr, size);
    }

    size_t new_size = (size + (align - 1)) & (~(align - 1));
    void *new_ptr = aligned_alloc(align, new_size);
    if (new_ptr != NULL)
    {
        size_t old_usable_size = new_size;
#ifdef __APPLE__
        old_usable_size = malloc_size(ptr);
#elif defined(__linux__)
        old_usable_size = malloc_usable_size(ptr);
#endif
        size_t copy_size = new_size < old_usable_size ? new_size : old_usable_size;
        if (ptr != NULL)
        {
            memcpy(new_ptr, ptr, copy_size);
            free(ptr);
        }
    }

    return new_ptr;
}
#endif//__UNIX__

#undef AN_REALLOC
#undef AN_FREE

#ifdef WITH_TBB
#define AN_ALLOC(size, align) scalable_aligned_malloc(size, align)
#define AN_REALLOC(p, size, align) scalable_aligned_realloc(p, size, align)
#define AN_FREE(p) scalable_aligned_free(p)
#elif defined(_WIN32)
#define AN_ALLOC(size, align) _aligned_malloc(size, align)
#define AN_REALLOC(p, size, align) _aligned_realloc(p, size, align)
#define AN_FREE(p) _aligned_free(p)
#else
#define AN_ALLOC(size, align) aligned_malloc(size, align)
#define AN_REALLOC(p, size, align) aligned_realloc_unix(p, size, align)
#define AN_FREE(p) aligned_free(p)
#endif //_WIN32

void *malloc_internal(size_t size, int align) {
    return AN_ALLOC(size, align);
}

void *realloc_internal(void *p, size_t size, int align) {
    return AN_REALLOC(p, size, align);
}

void *calloc_internal(size_t count, size_t size, int align) {
    void *p = AN_ALLOC(count * size, align);
    memset(p, 0, count * size);
    return p;
}

void free_internal(void *p) {
    AN_FREE(p);
}