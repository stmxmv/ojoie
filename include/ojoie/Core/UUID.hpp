// -*- Nitetrichor -*-
//===--------------------------- `target name` ---------------------------------===//
//
// UUID.hpp
//
// Created by Molybdenum on 11/16/21.
// Copyright (c) 2021 Nitetrichor. All rights reserved.
//===----------------------------------------------------------------------===//
#ifndef NITETRICHOR_UUID_HPP
#define NITETRICHOR_UUID_HPP

#include <ojoie/Configuration/typedef.h>
#include <string>

namespace AN {

class AN_API UUID {
public:
    enum {
        /// The number of bytes in a UUID's binary representation.
        Size = 16,

        /// The number of characters in a UUID's ToString representation.
        StringSize = 32,

        /// The number of bytes necessary to store a null-terminated UUID's ToString
        /// representation.
        StringBufferSize = StringSize + 1,
    };

    UInt32 data[4];

    /// Empty constructor.
    UUID() : data() {}

    /// Init will return false, if not valid
    bool Init();
    bool Init(const char *str);

    bool IsValid() const { return data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 0; }

    auto operator <=> (const UUID &) const = default;

    std::string ToString() const;
};

}



#endif//NITETRICHOR_UUID_HPP
