//
// Created by Aleudillonam on 5/12/2023.
//

#define MONO_DO_API(r, n, p)
#include "MonoLoader/MonoLoader.h"

#ifdef _WIN32
#include <Windows.h>
#pragma warning(disable: 4005)
#define MONO_LOADER_API __declspec(dllexport)
#else
#define MONO_LOADER_API
#endif//_WIN32

#include <cstdio>


static HMODULE gModule;

// define a pointer type for each API
#define MONO_DO_API(r,n,p)	typedef r (*PFN_##n) p;
#include "MonoLoader/MonoFunctions.h"

// declare storage for each API's function pointers
#define MONO_DO_API(r,n,p) extern "C" MONO_LOADER_API PFN_##n n = NULL;
#include "MonoLoader/MonoFunctions.h"

static void default_log(const char *msg) {
    puts(msg);
}

static MonoLoaderLogCallback gLogCallback = default_log;

void MonoLoaderSetLogCallback(MonoLoaderLogCallback callback) {
    gLogCallback = callback;
}

bool LoadMono(const char *libraryPath) {
    gModule = LoadLibraryA(libraryPath);

    if (gModule == NULL) {
        return false;
    }

#define LookupSymbol(sym) GetProcAddress(gModule, sym);

    bool funcsOK = true;
#define MONO_DO_API(r,n,p) n = (PFN_##n) LookupSymbol(#n); if (!n) { funcsOK = false; gLogCallback("mono: function " #n " not found"); }
#include "MonoLoader/MonoFunctions.h"


    if (!funcsOK) {
        UnloadMono();
    }

    return funcsOK;
}

void UnloadMono(void) {
    if (gModule) {
        FreeLibrary(gModule);
        gModule = NULL;
    }
}