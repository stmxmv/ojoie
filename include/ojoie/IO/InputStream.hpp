//
// Created by aojoie on 5/7/2023.
//

#ifndef OJOIE_INPUTSTREAM_HPP
#define OJOIE_INPUTSTREAM_HPP

namespace AN {

class InputStream {
public:

    virtual ~InputStream() = default;

    virtual int read(void *buffer, int size) = 0;

};

}

#endif//OJOIE_INPUTSTREAM_HPP
