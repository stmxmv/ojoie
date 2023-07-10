//
// Created by Aleudillonam on 9/13/2022.
//

#ifndef OJOIE_ACCESS_HPP
#define OJOIE_ACCESS_HPP

#include <utility>

namespace AN::Access {

/// Explicit instantiation definitions ignore member access specifiers:
/// parameter types and return types may be private
/// see https://en.cppreference.com/w/cpp/language/class_template#:~:text=Explicit%20instantiation%20definitions%20ignore%20member%20access%20specifiers%3A%20parameter%20types%20and%20return%20types%20may%20be%20private.
/// and https://stackoverflow.com/questions/424104/can-i-access-private-members-from-outside-the-class-without-using-friends

template<typename Tag, auto Member>
struct Accessor;

template<typename Tag, typename _Cls, typename T, T _Cls::*Member>
struct Accessor<Tag, Member> {

    typedef T ValueType;

    const T &get(const _Cls &obj) {
        return obj.*Member;
    }

    void set(_Cls &obj, T value) {
        obj.*Member = std::move(value);
    }

    friend auto getAccessorTypeImpl(Tag) { return Accessor{}; }
};

template<typename Tag>
struct TagBase {
    friend auto getAccessorTypeImpl(Tag);
};

template<typename Accessor>
struct AccessorTrait {
    using AccessorType = Accessor;
    using ValueType = typename Accessor::ValueType;
};

template<typename Tag>
struct TagTrait {
    using TagType = Tag;
    using AccessorType = decltype(getAccessorTypeImpl(std::declval<TagType>()));
    using ValueType = typename AccessorTrait<AccessorType>::ValueType;
};

template<typename Tag, typename _Cls, typename Accessor = typename TagTrait<Tag>::AccessorType>
auto get(const _Cls &obj) -> const typename AccessorTrait<Accessor>::ValueType & {
    return Accessor{}.get(obj);
}

template<typename Tag,
         typename _Cls,
         typename Accessor = typename TagTrait<Tag>::AccessorType,
         typename T>
void set(_Cls &obj, T &&value) {
    /// typename AccessorTrait<Accessor>::ValueType
    return Accessor{}.set(obj, std::forward<T>(value));
}

}

#endif//OJOIE_ACCESS_HPP
