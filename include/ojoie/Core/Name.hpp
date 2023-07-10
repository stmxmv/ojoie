//
// Created by aojoie on 4/12/2023.
//

#ifndef OJOIE_NAME_HPP
#define OJOIE_NAME_HPP

#include <ojoie/Configuration/typedef.h>
#include <string_view>

namespace AN {

class AN_API Name {

    int index;
    const char *_id;

public:
    Name() : _id() {}
    Name(const char *str); //NOLINT allow implicit conversion
    Name(std::string_view str); //NOLINT allow implicit conversion
    Name(const Name &) = default;
    Name(Name &&) = default;

    int getIndex() const { return index; }

    const char      *c_str() const { return _id; }
    std::string_view string_view() const { return _id; }

    bool  operator== (const Name &) const = default;
    Name &operator= (const Name &)        = default;

    bool operator== (const char *str) const { return string_view() == str; }
    bool operator== (std::string_view str) const { return string_view() == str; }

    Name &operator= (const char *str);
    Name &operator= (std::string_view str);
};

static_assert(sizeof(Name) == sizeof(const char *) + sizeof(UInt64));

}// namespace AN

template<>
struct std::hash<AN::Name> {
    size_t operator() (const AN::Name &name) const {
        return std::hash<const void *>()(name.c_str()); /// pointer is unique
    }
};

#endif//OJOIE_NAME_HPP
