//
// Created by Aleudillonam on 8/19/2023.
//

#include <ojoie/Utility/Log.h>
#include <ojoie/Utility/Assert.h>
#include <Export/mono/mono.h>

extern MonoDomain *gMonoDomain;
extern MonoAssembly *gOjoieAssembly;
extern MonoImage *gOjoieImage;

using namespace AN;

static void DebugLog_Injected(MonoString *msg) {
    char *str = mono_string_to_utf8(msg);
    AN_LOG(Debug, "%s", str);
    mono_free(str);
}

REGISTER_MONO_INTERNAL_CALL("AN.Debug::Log", DebugLog_Injected);

static void DebugAssert_Injected(MonoBoolean assertion) {
    ANAssert(assertion);
}

REGISTER_MONO_INTERNAL_CALL("AN.Debug::Assert", DebugAssert_Injected);