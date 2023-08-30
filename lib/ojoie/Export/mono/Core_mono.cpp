//
// Created by Aleudillonam on 8/16/2023.
//

#include <ojoie/Export/mono/Object_mono.h>
#include <ojoie/Core/App.hpp>
#include <ojoie/Core/Window.hpp>
#include <ojoie/Core/Actor.hpp>
#include <Export/mono/mono.h>

extern MonoDomain *gMonoDomain;
extern MonoAssembly *gOjoieAssembly;
extern MonoImage *gOjoieImage;

using namespace AN;

static uint32_t gcHandle = 0;

#define GET_IMPL(self, type) (*(type *) ((char *)self + sizeof(MonoObject)))
#define SET_IMPL(self, impl) ((*(void **) ((char *)self + sizeof(MonoObject))) = (void *)impl)


#pragma region Application

AN_API MonoObject *GetSharedApplication_Injected() {
    if (gcHandle) {
        return mono_gchandle_get_target(gcHandle);
    }

    MonoClass *appClass = mono_class_from_name(gOjoieImage, "AN", "Application");
    MonoObject* instance = mono_object_new(gMonoDomain, appClass);
    gcHandle = mono_gchandle_new(instance, true);

    return instance;
}

AN_API void FreeSharedApplicationGCHandle() {
    mono_gchandle_free(gcHandle);
    gcHandle = 0;
}

REGISTER_MONO_INTERNAL_CALL("AN.Application::Internal_GetSharedApplication_Injected", GetSharedApplication_Injected);

void ApplicationRun_Injected(MonoObject *object) {
    AN::Application::GetSharedApplication().run();
}

REGISTER_MONO_INTERNAL_CALL("AN.Application::Run", ApplicationRun_Injected);

#pragma endregion Application

static void Internal_RCRetain(MonoObject *self) {
    RefCounted<void> *rc = GET_IMPL(self, RefCounted<void> *);
    rc->retain();
}

REGISTER_MONO_INTERNAL_CALL("AN.RC::Internal_RCRetain",  Internal_RCRetain);

static void Internal_RCRelease(MonoObject *self) {
    RefCounted<void> *rc = GET_IMPL(self, RefCounted<void> *);
    rc->release();
}

REGISTER_MONO_INTERNAL_CALL("AN.RC::Internal_RCRelease",  Internal_RCRelease);

static void Internal_CreateWindow_Injected(MonoObject *self, Rect *rect, MonoBoolean wantsLayer) {
    Window *window = Window::Alloc();
    window->init(*rect, wantsLayer);
    SET_IMPL(self, window);
}

REGISTER_MONO_INTERNAL_CALL("AN.Window::Internal_CreateWindow", Internal_CreateWindow_Injected);

static void WindowMakeKeyAndOrderFront_Injected(MonoObject *self) {
    Window *window = GET_IMPL(self, Window *);
    window->makeKeyAndOrderFront();
}

REGISTER_MONO_INTERNAL_CALL("AN.Window::MakeKeyAndOrderFront", WindowMakeKeyAndOrderFront_Injected);

static void WindowCenter_Injected(MonoObject *self) {
    Window *window = GET_IMPL(self, Window *);
    window->center();
}

REGISTER_MONO_INTERNAL_CALL("AN.Window::Center", WindowCenter_Injected);

static void Internal_AllocInitActor(MonoObject *actor, MonoString *name) {
    AN::Actor *anActor = NewObject<Actor>();
    MonoHandleInit(anActor, actor);

    if (name) {
        char *str = mono_string_to_utf8(name);
        anActor->init(str);
        mono_free(str);
    } else {
        anActor->init();
    }
    SET_IMPL(actor, anActor);
}

REGISTER_MONO_INTERNAL_CALL("AN.Actor::Internal_AllocInitActor", Internal_AllocInitActor);
