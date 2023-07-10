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

/// win32 target arch define
#ifdef _WIN32
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define _AMD64_
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(__i386__) || defined(_M_IX86)
#define _X86_
#elif defined(__arm__) || defined(_M_ARM) || defined(_M_ARMT)
#define _ARM_
#endif
#endif//_WIN32


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

#endif //__cplusplus

typedef uint8_t UInt8;
typedef uint16_t UInt16;
typedef uint32_t UInt32;
typedef uint64_t UInt64;
typedef int8_t Int8;
typedef int16_t Int16;
typedef int32_t Int32;
typedef int64_t Int64;
typedef UInt32 ANFlags;

/// include some handy header
#include <ojoie/Export/Export.h>
#include <ojoie/Configuration/platform.h>
#include <ojoie/Utility/Assert.h>
#include <ojoie/Allocator/STLAllocator.hpp>

#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <map>
#include <set>

template<typename T>
using ANArray = std::vector<T, AN::STLAllocator<T, alignof(T)>>;

template<typename K, typename V>
using ANHashMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, AN::STLAllocator<std::pair<const K, V>, alignof(std::pair<const K, V>)>>;

template<typename K, typename V>
using ANHashMultiMap = std::unordered_multimap<K, V, std::hash<K>, std::equal_to<K>, AN::STLAllocator<std::pair<const K, V>, alignof(std::pair<const K, V>)>>;

template<typename T>
using ANHashSet = std::unordered_set<T, std::hash<T>, std::equal_to<T>, AN::STLAllocator<T, alignof(T)>>;

template<typename T>
using ANHashMultiSet = std::unordered_multiset<T, std::hash<T>, std::equal_to<T>, AN::STLAllocator<T, alignof(T)>>;

template<typename K, typename V>
using ANMap = std::map<K, V, std::less<K>, AN::STLAllocator<std::pair<const K, V>, alignof(std::pair<const K, V>)>>;

template<typename K, typename V>
using ANMultiMap = std::multimap<K, V, std::less<K>, AN::STLAllocator<std::pair<const K, V>, alignof(std::pair<const K, V>)>>;

template<typename T>
using ANSet = std::set<T, std::less<T>, AN::STLAllocator<T, alignof(T)>>;

template<typename T>
using ANMultiSet = std::multiset<T, std::less<T>, AN::STLAllocator<T, alignof(T)>>;

template<typename T>
using ANQueue = std::queue<T, std::deque<T, AN::STLAllocator<T, alignof(T)>>>;

template<typename T>
using ANStack = std::stack<T, std::deque<T, AN::STLAllocator<T, alignof(T)>>>;

#define OJOIE_USE_GLM

#endif//ALEUDILLONAM_TYPEDEFS_H
