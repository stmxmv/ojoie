//
// Created by aojoie on 5/8/2023.
//

#include <ojoie/Utility/Log.h>
//#include <mono/jit/jit.h>
//#include <mono/metadata/assembly.h>
//#include <mono/metadata/threads.h>
//#include <mono/utils/mono-logger.h>

#include <MonoLoader/MonoLoader.h>

#include <ojoie/Modules/Dylib.hpp>
#include <iostream>

#include <Windows.h>
using std::cout, std::endl;

extern "C"
__declspec(dllexport) void NativeFunction(void) {
    cout << "c# called native function" << endl;
}

void Mono_Debug_Log(MonoString *msg) {
    char *str = mono_string_to_utf8(msg);
    AN_LOG(Debug, "%s", str);
    mono_free(str);
}

void AN_MonoLogCallback(const char *log_domain,
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

void AN_MonoPringCallback(const char *string, mono_bool is_stdout) {
    AN_LOG(Warning, "[Mono] %s", string);
}

MonoString *DoSomething(MonoObject *self) {
    MonoClass *cls =  mono_object_get_class(self);
//    MonoClassField *field = mono_class_get_field_from_name(cls, "m_value");
//    int value = 13;
//    mono_field_set_value(self, field, &value);
    int *ptr = (int *) ((char *)self + sizeof(MonoObject));
    *ptr = 13;

    printf("self pointer %p\n", self);
    cout << "called from c#!!" << endl;

    return mono_string_new(mono_domain_get(), "string from c++");
}


//static bool HasAttribute(MonoClass* klass, MonoClassField *field, ScriptingClass* attributeClass)
//{
//    MonoCustomAttrInfo* attr = mono_custom_attrs_from_field (klass, field);
//
//    if (attr == NULL)
//        return false;
//
//    bool has = mono_custom_attrs_has_attr (attr, attributeClass);
//    mono_custom_attrs_free(attr);
//    return has;
//}
//

int main(int argc, const char * argv[]) {

    LoadMono("Data/Mono/mono-2.0-sgen.dll");
    MonoDomain *domain;

    mono_set_dirs("Data/Mono/lib","Data/Mono/etc");
    mono_trace_set_log_handler(AN_MonoLogCallback, nullptr);
//    mono_trace_set_print_handler(AN_MonoPringCallback);
//    mono_trace_set_printerr_handler(AN_MonoPringCallback);
    domain = mono_jit_init("com.an");
    MonoAssembly *assembly;

    const char *assembly_name = "Data/Mono/MonoAssembly.dll";
    assembly = mono_domain_assembly_open(domain, assembly_name);
    if (!assembly)
        exit(1);


    mono_add_internal_call("HelloWorld.Debug::Log", (void*) Mono_Debug_Log);
    mono_add_internal_call("HelloWorld.Test::DoSomething", (void *)DoSomething);

    MonoImage* monoImage = mono_assembly_get_image(assembly);
    MonoClass* entityClass = mono_class_from_name(monoImage,
                                                  "HelloWorld",
                                                  "Test");

    MonoObject* instance = mono_object_new(domain, entityClass);
    MonoMethod* constructorMethod = mono_class_get_method_from_name(entityClass, ".ctor", 0);
    MonoObject* exception = nullptr;
    if (constructorMethod) {
        mono_runtime_invoke(constructorMethod, instance, nullptr, &exception);
    }


    MonoMethod* processMethod = mono_class_get_method_from_name(entityClass, "SomeTest", 0);
    mono_runtime_invoke(processMethod, instance, nullptr, &exception);

//    int retval = mono_jit_exec(domain, assembly, 1, (char **)&assembly_name);
//    cout << "return value " << retval << endl;

    mono_jit_cleanup(domain);


    UnloadMono();

    return 0;
}