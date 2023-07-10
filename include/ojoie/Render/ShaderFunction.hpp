//
// Created by aojoie on 4/17/2023.
//

#ifndef OJOIE_SHADERFUNCTION_HPP
#define OJOIE_SHADERFUNCTION_HPP

#include <ojoie/Configuration/typedef.h>

namespace AN::RC {

class ShaderFunctionImpl {
public:
    virtual bool init(const UInt8 *code, size_t size) = 0;
    virtual void deinit() = 0;
    virtual ~ShaderFunctionImpl() = default;
};

class ShaderFunction : private NonCopyable {
    ShaderFunctionImpl *impl;
public:

    ShaderFunction() : impl() {}

    ShaderFunction(ShaderFunction &&other) noexcept : impl(other.impl) {
        other.impl = nullptr;
    }

    ShaderFunctionImpl *getImpl() const { return impl; }

    ~ShaderFunction() {
        deinit();
    }

    bool init(const UInt8 *code, size_t size);

    void deinit();
};


}

#endif//OJOIE_SHADERFUNCTION_HPP
