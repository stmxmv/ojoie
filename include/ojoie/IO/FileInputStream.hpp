//
// Created by aojoie on 5/7/2023.
//

#ifndef OJOIE_FILEINPUTSTREAM_HPP
#define OJOIE_FILEINPUTSTREAM_HPP

#include <ojoie/IO/InputStream.hpp>
#include <ojoie/HAL/File.hpp>

namespace AN {

class AN_API FileInputStream : public InputStream {

    File *_file;

public:

    explicit FileInputStream(File &file) : _file(&file) {}

    virtual int read(void *buffer, int size) override {
        return _file->read(buffer, size);
    }

};


}

#endif//OJOIE_FILEINPUTSTREAM_HPP
