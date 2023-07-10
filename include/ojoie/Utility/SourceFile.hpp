// -*- Ang -*-
//===--------------------------- `target name` ---------------------------------===//
//
// SourceFile.hpp
// include/Basic
// Created by Molybdenum on 3/7/23.
//===----------------------------------------------------------------------===//

#ifndef ANG_SOURCEFILE_HPP
#define ANG_SOURCEFILE_HPP

#include <ojoie/Configuration/typedef.h>
#include <ojoie/Core/Exception.hpp>
#include <string_view>


namespace AN {

/// \brief SourceFile manages the mapping of the file into memory
class AN_API SourceFile : private NonCopyable {

    const char *_buffer{};

    struct Impl;
    Impl *impl;

public:

    SourceFile();

    /// equivalent to open
    bool init(std::string_view path, Error *error = nullptr);

    bool open(std::string_view path, Error *error = nullptr);

    ~SourceFile();

    const char *getBuffer() {
        return _buffer;
    }

    void close();

};


}

#endif//ANG_SOURCEFILE_HPP
