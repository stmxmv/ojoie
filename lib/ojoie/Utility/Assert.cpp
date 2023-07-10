//
// Created by aojoie on 4/1/2023.
//

#include "Utility/Assert.h"
#include "Configuration/typedef.h"
#include "Utility/Log.h"

void __an_assert_fail() {
#ifdef AN_DEBUG
    debugBreak();
#endif
}

#ifdef __cpp_lib_source_location
void __an_assert_log(const char *expect, std::source_location location) {
    AN_LOG(Assert, "ANAssertion %s Failed in file %s (%u:%u) function: %s",
           expect, location.file_name(), location.line(), location.column(), location.function_name());
}

void __an_assert_log(const char *expect, const char *msg, std::source_location location) {
    AN_LOG(Assert, "ANAssertion %s Failed in file %s (%u:%u) function: %s message: %s",
           expect, location.file_name(), location.line(), location.column(), location.function_name(), msg);
}
#endif//__cpp_lib_source_location

void __an_assert_log(const char *expect, const char *file, int line, const char *func) {
    AN_LOG(Assert, "ANAssertion %s Failed in file %s line: %d function: %s", expect, file, line, func);
}

void __an_assert_log(const char *expect, const char *file, int line, const char *func, const char *msg) {
    AN_LOG(Assert, "ANAssertion %s Failed in file %s line: %d function: %s message: %s", expect, file, line, func, msg);
}