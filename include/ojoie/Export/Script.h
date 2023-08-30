//
// Created by Aleudillonam on 8/18/2023.
//

#pragma once

#include <ojoie/Export/mono/mono.h>

inline void ScriptRuntimeInit() {
    MonoRuntimeInit();
}

inline void ScriptRuntimeCleanup() {
    MonoRuntimeCleanup();
}
