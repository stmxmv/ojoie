//
// Created by Aleudillonam on 8/16/2023.
//

#pragma once
#include <ojoie/Export/Export.h>
#include <MonoLoader/MonoLoader.h>

AN_API void MonoRuntimeInit();
AN_API void MonoRuntimeCleanup();

AN_API MonoDomain *GetOjoieDomain();
AN_API MonoImage *GetOjoieImage();
AN_API MonoAssembly *GetOjoieAssembly();

#define REGISTER_MONO_INTERNAL_CALL(name, func) \
    struct func##_InternalCall_Register { \
        func##_InternalCall_Register() {  \
            RegisterMonoInternalCall(name, (void *)func);\
        }\
    }  s_##func##_InternalCall_Register


/// call this function before application init which means before main, use macro above
AN_API void RegisterMonoInternalCall(const char *name, void *func);
