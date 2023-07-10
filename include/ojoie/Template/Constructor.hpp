//
// Created by aojoie on 4/1/2023.
//

#ifndef OJOIE_CONSTRUCTOR_HPP
#define OJOIE_CONSTRUCTOR_HPP

#include <concepts>

namespace AN {

template<typename T, bool _abstract = std::is_abstract_v<T>>
struct Constructor {

    static T *NewtStatic() {
        return new T();
    }

    template<typename ..._Args>
    static void ConstructStatic(void *mem, _Args &&...args) {
        new (mem) T(std::forward<_Args>(args)...);
    }

    template<typename ..._Args>
    static void ConstructStatic(void *mem, _Args ...args) {
        new (mem) T(args...);
    }
};

template<typename T>
struct Constructor<T, true> {

    inline constexpr static T (*NewtStatic)() = nullptr;

    template<typename ..._Args>
    inline constexpr static void (*ConstructStatic)(void *, _Args ...) = nullptr;

};

}

#endif//OJOIE_CONSTRUCTOR_HPP
