//
// Created by aojoie on 4/12/2023.
//

#ifndef OJOIE_MEMORYDEFINES_H
#define OJOIE_MEMORYDEFINES_H

#include <ojoie/Allocator/MemoryManager.h>

enum {
#if AN_OSX || AN_IPHONE || AN_ANDROID || AN_PS3 || AN_XENON || AN_BB10 || AN_TIZEN
    kDefaultMemoryAlignment = 16
#else
    kDefaultMemoryAlignment = sizeof(void *)
#endif
};

#define AN_MALLOC(size)                       malloc_internal(size, kDefaultMemoryAlignment)
#define AN_CALLOC(count, size)                calloc_internal(count, size, kDefaultMemoryAlignment)
#define AN_REALLOC(p, size)                   realloc_internal(p, size, kDefaultMemoryAlignment)
#define AN_MALLOC_ALIGNED(size, align)        malloc_internal(size, align)
#define AN_CALLOC_ALIGNED(count, size, align) calloc_internal(count, size, align)
#define AN_REALLOC_ALIGNED(p, size, align)    realloc_internal(p, size, align)
#define AN_FREE(p)                            free_internal(p)

#ifdef __cplusplus

template<typename T>
void ANSafeFree(T *&ptr) {
    if (ptr) {
        AN_FREE(ptr);
        ptr = nullptr;
    }
}

#endif//__cplusplus

#endif//OJOIE_MEMORYDEFINES_H
