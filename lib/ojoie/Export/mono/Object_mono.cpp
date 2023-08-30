//
// Created by Aleudillonam on 8/19/2023.
//

#include <ojoie/Export/mono/Object_mono.h>
#include <ojoie/Object/Object.hpp>
#include <Export/mono/mono.h>

#define OBJECT_GET_IMPL(self, type) (*(type *) ((char *)self + sizeof(MonoObject)))
#define OBJECT_SET_IMPL(self, impl) ((*(void **) ((char *)self + sizeof(MonoObject))) = (void *)impl)


struct MonoObjectInfo {
    MonoObject *monoObject;
    uint32_t    gcHandle;
};

void MonoHandleInit(AN::Object *anObject, MonoObject *object) {
    uint32_t gcHandle = mono_gchandle_new(object, true);
    MonoObjectInfo *info = new MonoObjectInfo();
    info->gcHandle = gcHandle;
    info->monoObject = object;
    anObject->setScriptHandle((intptr_t) info);
}

void MonoObjectHandleCleanup(intptr_t handle) {
    if (handle) {
        MonoObjectInfo *info = (MonoObjectInfo *) handle;
        mono_gchandle_free(info->gcHandle);
        delete info;
    }
}

MonoObject *ObjectToMono(AN::Object *obj) {
    if (obj->getScriptHandle() == 0) {
        MonoClass *cls = mono_class_from_name(GetOjoieImage(), "AN", obj->getClassName());
        MonoObject *monoObject = mono_object_new(GetOjoieDomain(), cls);
        uint32_t gcHandle = mono_gchandle_new(monoObject, true);
        MonoObjectInfo *info = new MonoObjectInfo();
        info->monoObject = monoObject;
        info->gcHandle = gcHandle;
        obj->setScriptHandle((intptr_t) info);
        return monoObject;
    }
    MonoObjectInfo *info = (MonoObjectInfo *) obj->getScriptHandle();
    return info->monoObject;
}

static void Internal_AllocInitObject(MonoObject *object) {
    AN::Object *anObject = AN::NewObject<AN::Object>();
    MonoHandleInit(anObject, object);
    OBJECT_SET_IMPL(object, anObject);
    anObject->init();
}

REGISTER_MONO_INTERNAL_CALL("AN.Object::Internal_AllocInitObject", Internal_AllocInitObject);

static void DestroyImmediate_Injected(MonoObject *object) {
    AN::Object *anObject = OBJECT_GET_IMPL(object, AN::Object *);
    AN::DestroyObject(anObject);
}

REGISTER_MONO_INTERNAL_CALL("AN.Object::DestroyImmediate", DestroyImmediate_Injected);


static MonoString *DebugDescription_Injected(MonoObject *self) {
    AN::Object *anObject = OBJECT_GET_IMPL(self, AN::Object *);
    std::string desc = anObject->debugDescription();
    return mono_string_new(GetOjoieDomain(), desc.c_str());
}

REGISTER_MONO_INTERNAL_CALL("AN.Object::DebugDescription", DebugDescription_Injected);
