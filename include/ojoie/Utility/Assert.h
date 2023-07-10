//
// Created by aojoie on 4/1/2023.
//

#ifndef OJOIE_ASSERT_H
#define OJOIE_ASSERT_H

#include <ojoie/Export/Export.h>

#ifdef __cplusplus
#include <iostream>
#ifdef __cpp_lib_source_location
#include <source_location>
#endif
#endif

AN_API void __an_assert_log(const char *expect, const char *file, int line, const char *func);
AN_API void __an_assert_log(const char *expect, const char *file, int line, const char *func, const char *msg);

AN_API void __an_assert_fail();

#ifdef __cpp_lib_source_location

AN_API void __an_assert_log(const char *expect, std::source_location location = std::source_location::current());
AN_API void __an_assert_log(const char *expect,  const char *msg, std::source_location location = std::source_location::current());


#define ANAssert(expect, ...) do { \
        if (!(expect)) {            \
            __an_assert_log(#expect, ##__VA_ARGS__); \
            __an_assert_fail(); \
        }\
    } while (0)

#else

#define ANAssert(expect, ...) do { \
        if (!(expect)) {           \
            __an_assert_log(#expect, __FILE__, __LINE__, __func__,  ##__VA_ARGS__); \
            __an_assert_fail(); \
        }\
    } while (0)

#endif

#define ANAssertFail(msg) do { \
        __an_assert_log("", msg); \
        __an_assert_fail(); \
    } while (0)

#endif//OJOIE_ASSERT_H
