//
// Created by Aleudillonam on 5/2/2023.
//

#ifndef OJOIE_DYLIB_HPP
#define OJOIE_DYLIB_HPP

#include <ojoie/Export/Export.h>

AN_API void *LoadDynamicLibrary(const char *absolutePath);
AN_API void *LoadAndLookupSymbol(const char *absolutePath, const char *name);
AN_API void  UnloadDynamicLibrary(void *libraryReference);
AN_API void  UnloadDynamicLibrary(const char *absolutePath);
AN_API bool  LoadAndLookupSymbols(const char *path, ...);
AN_API void *LookupSymbol(void *libraryReference, const char *symbolName);

#endif//OJOIE_DYLIB_HPP
