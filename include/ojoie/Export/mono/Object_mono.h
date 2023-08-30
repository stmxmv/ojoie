//
// Created by Aleudillonam on 8/23/2023.
//

#pragma once

#include <ojoie/Object/Object.hpp>
#include <ojoie/Export/mono/mono.h>

MonoObject *ObjectToMono(AN::Object *obj);

void MonoHandleInit(AN::Object *anObject, MonoObject *object);

void MonoObjectHandleCleanup(intptr_t handle);