//
// Created by Aleudillonam on 5/12/2023.
//

#ifndef OJOIE_MONOSTRUCTS_H
#define OJOIE_MONOSTRUCTS_H

typedef struct MonoVTable MonoVTable;
typedef struct _MonoThreadsSync MonoThreadsSync;

struct _MonoObject {
    MonoVTable *vtable;
    MonoThreadsSync *synchronisation;
};

#define MONO_ALLOCATOR_VTABLE_VERSION 1

typedef struct {
    int version;
    void *(*malloc)      (size_t size);
    void *(*realloc)     (void *mem, size_t count);
    void (*free)        (void *mem);
    void *(*calloc)      (size_t count, size_t size);
} MonoAllocatorVTable;

#endif//OJOIE_MONOSTRUCTS_H
