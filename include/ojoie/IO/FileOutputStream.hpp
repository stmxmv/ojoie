//
// Created by aojoie on 5/7/2023.
//

#ifndef OJOIE_FILEOUTPUTSTREAM_HPP
#define OJOIE_FILEOUTPUTSTREAM_HPP

#include <ojoie/IO/OutputStream.hpp>
#include <ojoie/HAL/File.hpp>

namespace AN {

class FileOutputStream : public OutputStream {

    File *_file;

public:

    explicit FileOutputStream(File &file) : _file(&file) {}

    virtual bool write(const void *buffer, int size) override {
        return _file->write(buffer, size);
    }

};

}

#endif//OJOIE_FILEOUTPUTSTREAM_HPP
