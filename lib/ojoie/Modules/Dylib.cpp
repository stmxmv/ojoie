//
// Created by Aleudillonam on 5/2/2023.
//

#include "Modules/Dylib.hpp"
#include "Configuration/platform.h"
#include "Configuration/typedef.h"
#include "Utility/Log.h"

#include <string>
#include <unordered_map>
#include <filesystem>

#if AN_WIN
#include <libloaderapi.h>
#endif

static std::unordered_map<std::string, void *> gLoaded;

void *LoadDynamicLibrary(const char *absolutePath) {
    void *library = nullptr;

    std::filesystem::path path(absolutePath);
    if (!path.has_extension()) {
#if AN_WIN
        path.replace_extension("dll");
#endif
    }

    std::string pathString = path.string();

    // load library if needed
    if (gLoaded.find(pathString) != gLoaded.end()) {
        library = gLoaded[pathString];
    } else {

#if AN_WIN

        HINSTANCE module = LoadLibraryW(path.c_str());

#if AN_DEBUG
        /// try to find debug version
        if (module == nullptr) {
            std::string debugPathStr = path.string();
            debugPathStr.insert(debugPathStr.rfind('.'), "d");
            path = debugPathStr;
            module = LoadLibraryW(path.c_str());
        }
#endif//AN_DEBUG

        library             = module;
#endif//AN_WIN

        if (library) {
            gLoaded[pathString] = library;
        }
    }

    return library;
}

void *LoadAndLookupSymbol(const char *absolutePath, const char *name) {
    void* library = LoadDynamicLibrary(absolutePath);
    void* symbol = LookupSymbol(library, name);
    return symbol;
}

void UnloadDynamicLibrary(void *libraryReference) {
    for (auto it = gLoaded.begin(); it != gLoaded.end(); ++it) {
        if (it->second == libraryReference) {
#if AN_WIN
            FreeLibrary( (HMODULE)it->second );
#endif
            gLoaded.erase(it);
            break;
        }
    }
}


void UnloadDynamicLibrary(const char *absolutePath) {
    if (gLoaded.contains(absolutePath) && gLoaded[absolutePath]) {
#if AN_WIN
        FreeLibrary( (HMODULE)gLoaded[absolutePath] );
#endif
    }
    gLoaded.erase (absolutePath);
}

bool LoadAndLookupSymbols(const char *path, ...) {
    va_list ap;
    va_start (ap, path);
    for (;;) {
        const char* symbolName = va_arg(ap, const char*);
        if (symbolName == nullptr)
            return true;

        void** functionHandle = va_arg(ap, void**);
        ANAssert(functionHandle != nullptr);

        *functionHandle = LoadAndLookupSymbol (path, symbolName);
        if (*functionHandle == NULL)
            return false;
    }

    return false;
}

void *LookupSymbol(void *libraryReference, const char *symbolName) {
#if AN_WIN

    if (!libraryReference)
        return nullptr;
    return (void *)GetProcAddress((HMODULE)libraryReference, symbolName);

#else
    AN_LOG(Error, "LoadAndLookupSymbol is not supported on this platform.\n");
    return nullptr;
#endif
}
