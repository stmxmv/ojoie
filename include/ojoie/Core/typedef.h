// -*- Aleudillonam -*-
//===--------------------------- Aleudillonam ------------------------------===//
//
// typedefs.h
// include/Aleudillonam
// Created by Molybdenum on 2/12/22.
//===----------------------------------------------------------------------===//

#ifndef ALEUDILLONAM_TYPEDEFS_H
#define ALEUDILLONAM_TYPEDEFS_H

#if defined(VLD_MEM_CHECK)
#include <vld.h>
#endif

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#endif

// Should always inline no matter what.
#ifndef __always_inline
#if defined(__GNUC__)
#define __always_inline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define __always_inline __forceinline
#else
#define __always_inline inline
#endif
#endif


#ifdef _MSC_VER
#   ifdef AN_BUILD_OJOIE
#       define AN_EXPORT __declspec(dllexport)
#   else
#       define AN_EXPORT __declspec(dllimport)
#   endif
#else
#   define AN_IMPORT
#   define AN_EXPORT
#endif


// Should always inline, except in dev builds because it makes debugging harder.
#ifndef __force_inline
#if AN_DEBUG
#define __force_inline inline
#else
#define __force_inline __always_inline
#endif
#endif

#ifndef __noreturn
#if defined(__GNUC__)
#define __noreturn __attribute__((noreturn))
#elif defined(_MSC_VER)
#define __noreturn __declspec(noreturn)
#endif
#endif


#ifndef __asm_volatile_goto
#define __asm_volatile_goto(...) asm goto(__VA_ARGS__)
#endif


#ifndef __noinline
#if defined(__GNUC__)
#define __noinline __attribute__((noinline))
#elif defined(_MSC_VER)
#define __noinline __declspec(noinline)

#endif

#endif



#ifndef __stringify_1
#define __stringify_1(...) #__VA_ARGS__
#endif
#ifndef __stringify
#define __stringify(...) __stringify_1(__VA_ARGS__)
#endif


#define AN_CAT_IMPL(a, b) a##b
#define AN_CAT(a, b)      AN_CAT_IMPL(a, b)


#ifndef __same_type
#if defined(__GNUC__)

#ifdef __cplusplus

#include <type_traits>// for std::is_same
#define __same_type(a, b) std::is_same<typeof(a), typeof(b)>::value

#else

#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#endif
#elif defined(_MSC_VER)
#define __same_type(a, b) __decltype(a) == __decltype(b)
#else
#error "don't know how to implement __same_type for this compiler. Abort! Abort!"
#endif
#endif



#if __x86_64__ || _M_AMD64

#define cpu_relax() rep_nop()

#ifdef _WIN32
#include <intrin.h>
#endif
static __always_inline void rep_nop() {
#ifdef _WIN32
    _mm_pause();
#else
    asm volatile("rep;nop" ::
                         : "memory");
#endif
}

static __always_inline void debugBreak() {
#ifdef _WIN32
    __debugbreak();
#else
    asm volatile("int3");
#endif
}

#elif __arm64__ || _M_ARM

static __always_inline void cpu_relax() {
    asm volatile("yield" ::
                         : "memory");
}

static __always_inline void debugBreak() {
    asm volatile("brk 0");
}

#else

#define cpu_relax()
#define debugBreak()
#endif


#ifdef __cplusplus

#include <iostream>
#ifdef __cpp_lib_source_location
#include <source_location>
#endif

namespace AN {
struct NonCopyable {
protected:
    NonCopyable()                               = default;
    ~NonCopyable()                              = default;
    NonCopyable(NonCopyable &&)                 = default;
    NonCopyable &operator=(NonCopyable &&)      = default;
public:
    NonCopyable(const NonCopyable &)            = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
};


/// \note https://cppinsights.io

#ifdef __clang__
template<typename, typename ...>
struct [[deprecated]] dump {};

#else
template<typename, typename ...>
struct dump;
#endif

template<typename ...T>
void runtime_dump() {
    std::cout <<
#ifdef __GNUC__
            std::string_view(__PRETTY_FUNCTION__).substr(5)
#elif defined(_MSC_VER)
            __FUNCSIG__
#else
#error "no runtime_dump() implementation for this compiler"
#endif
              << '\n';
}


template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

template<typename E>
struct enable_bitmask_operators : std::false_type {};

}

template<typename E>
auto operator|(E lhs,E rhs) -> typename std::enable_if<AN::enable_bitmask_operators<E>::value, E>::type {
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename E>
auto operator&(E lhs,E rhs) -> typename std::enable_if<AN::enable_bitmask_operators<E>::value, E>::type {
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template<typename E>
auto operator^(E lhs,E rhs) -> typename std::enable_if<AN::enable_bitmask_operators<E>::value, E>::type {
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template<typename E>
auto operator~(E e) -> typename std::enable_if<AN::enable_bitmask_operators<E>::value, E>::type {
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(~static_cast<underlying>(e));
}

template<typename E>
auto operator&=(E &lhs,E rhs) -> typename std::enable_if<AN::enable_bitmask_operators<E>::value, E>::type {
    typedef typename std::underlying_type<E>::type underlying;
    lhs = lhs & rhs;
    return lhs;
}


template<typename E>
auto operator|=(E &lhs,E rhs) -> typename std::enable_if<AN::enable_bitmask_operators<E>::value, E>::type {
    typedef typename std::underlying_type<E>::type underlying;
    lhs = lhs | rhs;
    return lhs;
}


template<typename E>
auto operator^=(E &lhs,E rhs) -> typename std::enable_if<AN::enable_bitmask_operators<E>::value, E>::type {
    typedef typename std::underlying_type<E>::type underlying;
    lhs = lhs ^ rhs;
    return lhs;
}

#endif


#ifdef __cplusplus
extern "C"
#endif
void ANLog(const char * fmt, ...);

#ifdef __cpp_lib_source_location

inline void __assert_log(const char *expect, std::source_location location = std::source_location::current()) {
    ANLog("ANAssertion %s Failed in file %s (%u:%u) function: %s", expect, location.file_name(), location.line(), location.column(), location.function_name());
}

inline void __assert_log(const char *expect,  const char *msg, std::source_location location = std::source_location::current()) {
    ANLog("ANAssertion %s Failed in file %s (%u:%u) function: %s message: %s", expect, location.file_name(), location.line(), location.column(), location.function_name(), msg);
}

#define ANAssert(expect, ...) do { \
        if (!(expect)) {            \
            __assert_log(#expect, ##__VA_ARGS__);\
        }\
    } while (0)
#else

inline void __assert_log(const char *expect, const char *file, int line, const char *func) {
    ANLog("ANAssertion %s Failed in file %s line: %d function: %s", expect, file, line, func);
}

inline void __assert_log(const char *expect, const char *file, int line, const char *func, const char *msg) {
    ANLog("ANAssertion %s Failed in file %s line: %d function: %s message: %s", expect, file, line, func, msg);
}

#define ANAssert(expect, ...) do { \
        if (!(expect)) {           \
            __assert_log(#expect, __FILE__, __LINE__, __func__,  ##__VA_ARGS__);\
        }\
    } while (0)

#endif





#define OJOIE_USE_GLM

#define OJOIE_USE_GLFW

#ifdef __APPLE__
#   define OJOIE_USE_METAL
#else
#   define OJOIE_USE_VULKAN
#endif


#endif//ALEUDILLONAM_TYPEDEFS_H
