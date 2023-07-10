//
// Created by aojoie on 4/12/2023.
//

#ifndef OJOIE_MEMORYMANAGER_H
#define OJOIE_MEMORYMANAGER_H

#include <ojoie/Export/Export.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

AN_API void *malloc_internal(size_t size, int align);
AN_API void *realloc_internal(void *, size_t size, int align);
AN_API void *calloc_internal(size_t count, size_t size, int align);
AN_API void  free_internal(void *);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//OJOIE_MEMORYMANAGER_H
