//
// Created by Aleudillonam on 5/12/2023.
//

#ifdef __cplusplus

#include <MonoLoader/MonoStructs.h>
#include <cstdint>

#else
#include <stdbool.h>
#include <stdint.h>
#endif//__cplusplus

typedef int32_t   mono_bool;
typedef uint8_t   mono_byte;
typedef mono_byte MonoBoolean;

typedef struct _MonoType MonoType;
typedef struct _MonoDomain MonoDomain;
typedef struct _MonoAssembly MonoAssembly;
typedef struct _MonoImage MonoImage;
typedef struct _MonoClass MonoClass;
typedef struct _MonoObject MonoObject;
typedef struct _MonoMethod MonoMethod;
typedef struct _MonoString MonoString;
typedef struct _MonoArray MonoArray;
typedef struct _MonoClassField MonoClassField;
typedef struct _MonoProperty MonoProperty;

typedef struct _MonoReflectionAssembly MonoReflectionAssembly;
typedef struct _MonoReflectionType MonoReflectionType;

typedef void (*MonoPrintCallback) (const char *string, mono_bool is_stdout);
typedef void (*MonoLogCallback) (const char *log_domain, const char *log_level, const char *message, mono_bool fatal, void *user_data);

#ifndef MONO_DO_API
#define MONO_DO_API(r, n, p)
#endif//MONO_DO_API

MONO_DO_API(mono_bool, mono_set_allocator_vtable, (MonoAllocatorVTable* vtable))

MONO_DO_API(void, mono_set_dirs, (const char *assembly_dir, const char *config_dir))
MONO_DO_API(MonoDomain *, mono_jit_init, (const char *file))
MONO_DO_API(MonoAssembly *, mono_domain_assembly_open, (MonoDomain *domain, const char *name))
MONO_DO_API(void, mono_add_internal_call, (const char *name, const void* method))
MONO_DO_API(MonoImage *, mono_assembly_get_image, (MonoAssembly *assembly))
MONO_DO_API(MonoClass *, mono_class_from_name, (MonoImage *image, const char* name_space, const char *name))
MONO_DO_API(MonoObject *, mono_object_new, (MonoDomain *domain, MonoClass *klass))
MONO_DO_API(MonoMethod *, mono_class_get_method_from_name, (MonoClass *klass, const char *name, int param_count))
MONO_DO_API(MonoObject*, mono_runtime_invoke, (MonoMethod *method, void *obj, void **params, MonoObject **exc))
MONO_DO_API(void, mono_jit_cleanup, (MonoDomain *domain))
MONO_DO_API(char *, mono_string_to_utf8, (MonoString *string_obj))
MONO_DO_API(void, mono_free, (void *))
MONO_DO_API(MonoDomain *, mono_domain_get, (void))
MONO_DO_API(MonoClass*, mono_object_get_class, (MonoObject *obj))
MONO_DO_API(MonoString*, mono_string_new, (MonoDomain *domain, const char *text))
MONO_DO_API(void, mono_trace_set_log_handler, (MonoLogCallback callback, void *user_data))
MONO_DO_API(MonoClassField *, mono_class_get_field_from_name, (MonoClass *klass, const char *name))
MONO_DO_API(void, mono_field_get_value, (MonoObject *obj, MonoClassField *field, void *value))
MONO_DO_API(void, mono_field_set_value, (MonoObject *obj, MonoClassField *field, void *value))

MONO_DO_API(uint32_t, mono_gchandle_new, (MonoObject *obj, mono_bool pinned))
MONO_DO_API(uint32_t, mono_gchandle_new_weakref, (MonoObject *obj, mono_bool track_resurrection))
MONO_DO_API(MonoObject*, mono_gchandle_get_target, (uint32_t gchandle))
MONO_DO_API(void, mono_gchandle_free, (uint32_t gchandle))
MONO_DO_API(void*, mono_object_unbox, (MonoObject *obj))
MONO_DO_API(MonoObject *, mono_object_castclass_mbyref, (MonoObject *obj, MonoClass *klass))

MONO_DO_API(MonoProperty *, mono_class_get_property_from_name, (MonoClass *klass, const char *name))
MONO_DO_API(void, mono_property_set_value, (MonoProperty *prop, void *obj, void **params, MonoObject **exc))
MONO_DO_API(MonoObject*, mono_property_get_value, (MonoProperty *prop, void *obj, void **params, MonoObject **exc))

MONO_DO_API(MonoClass*, mono_type_get_class, (MonoType *type))

MONO_DO_API(MonoReflectionAssembly*, mono_assembly_get_object, (MonoDomain *domain, MonoAssembly *assembly))
MONO_DO_API(MonoType*, mono_reflection_type_get_type, (MonoReflectionType *reftype))