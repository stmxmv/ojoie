//
// Created by Aleudillonam on 5/12/2023.
//

#ifndef OJOIE_MONOLOADER_H
#define OJOIE_MONOLOADER_H

#ifdef __cplusplus

#include <cstdint>
extern "C" {

#else

#include <stdbool.h>
#include <stdint.h>
#endif//__cplusplus

typedef void (*MonoLoaderLogCallback)(const char *errorMsg);

void MonoLoaderSetLogCallback(MonoLoaderLogCallback callback);
bool LoadMono(const char *libraryPath);
void UnloadMono(void);


#ifdef __cplusplus
}
#endif//__cplusplus

#ifdef _WIN32
#define MONO_LOADER_API extern "C" __declspec(dllimport)
#else
#define MONO_LOADER_API extern "C"
#endif//_WIN32


#ifndef MONO_DO_API
#define MONO_DO_API(r,n,p) MONO_LOADER_API r (*n) p;
#endif//MONO_DO_API
#include <MonoLoader/MonoFunctions.h>

#endif//OJOIE_MONOLOADER_H
