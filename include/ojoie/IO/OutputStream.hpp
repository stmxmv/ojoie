//
// Created by aojoie on 5/7/2023.
//

#ifndef OJOIE_OUTPUTSTREAM_HPP
#define OJOIE_OUTPUTSTREAM_HPP

namespace AN {

class OutputStream {
public:

    virtual ~OutputStream() = default;

    virtual bool write(const void *buffer, int size) = 0;

};


}

#endif//OJOIE_OUTPUTSTREAM_HPP
