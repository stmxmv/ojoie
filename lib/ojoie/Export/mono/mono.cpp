//
// Created by Aleudillonam on 8/16/2023.
//

#include "Export/mono/mono.h"
#include "Export/mono/Object_mono.h"
#include "Utility/Log.h"

static void AN_MonoLogCallback(const char *log_domain,
                        const char *log_level,
                        const char *message,
                        mono_bool fatal,
                        void *user_data) {

    if (fatal) {
        AN_LOG(Error, "%s", message);
    } else {
        AN_LOG(Log, "[Mono] [%s] %s", log_level, message);
    }
}

AN_API MonoDomain *gMonoDomain;
AN_API MonoAssembly *gOjoieAssembly;
AN_API MonoImage *gOjoieImage;

static std::vector<std::pair<const char *, void *>> &s_GetInternalCallMap() {
    static std::vector<std::pair<const char *, void *>> map;
    return map;
}

void MonoRuntimeInit() {
    LoadMono("Data/Mono/mono-2.0-sgen.dll");

    MonoAllocatorVTable monoAllocator;
    monoAllocator.version = MONO_ALLOCATOR_VTABLE_VERSION;
    monoAllocator.malloc = [](size_t size) {
        return AN_MALLOC(size);
    };
    monoAllocator.realloc = [](void *ptr, size_t size) {
        return AN_REALLOC(ptr, size);
    };
    monoAllocator.calloc = [](size_t count, size_t size) {
        return AN_CALLOC(count, size);
    };
    monoAllocator.free = free_internal;
    if (!mono_set_allocator_vtable(&monoAllocator)) {
        AN_LOG(Info, "Fail to install mono allocator vtable, maybe version is different");
    }

    mono_set_dirs("Data/Mono/lib","Data/Mono/etc");
    mono_trace_set_log_handler(AN_MonoLogCallback, nullptr);

    gMonoDomain = mono_jit_init("com.an");

    /// ojoie runtime assembly

    const char *assembly_name = "Data/Mono/OjoieAssembly.dll";
    gOjoieAssembly = mono_domain_assembly_open(gMonoDomain, assembly_name);
    if (gOjoieAssembly == nullptr) {
        AN_LOG(Error, "Unable to load ojoie mono assembly");
    }

    gOjoieImage = mono_assembly_get_image(gOjoieAssembly);

    /// register internal calls
    auto &map = s_GetInternalCallMap();
    for (auto [name, func] : map) {
        mono_add_internal_call(name, func);
    }
    /// free memory
    map = {};


    AN::Object::SetScriptHandleCleanupFunc(MonoObjectHandleCleanup);
}

void MonoRuntimeCleanup() {
    mono_jit_cleanup(gMonoDomain);
    UnloadMono();
}

void RegisterMonoInternalCall(const char *name, void *func) {
    s_GetInternalCallMap().emplace_back(name, func);
}


MonoDomain *GetOjoieDomain() {
    return gMonoDomain;
}

MonoImage *GetOjoieImage() {
    return gOjoieImage;
}

MonoAssembly *GetOjoieAssembly() {
    return gOjoieAssembly;
}
