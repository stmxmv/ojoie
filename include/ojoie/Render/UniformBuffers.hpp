//
// Created by aojoie on 5/4/2023.
//

#ifndef OJOIE_UNIFORMBUFFERS_HPP
#define OJOIE_UNIFORMBUFFERS_HPP

#include <ojoie/Render/RenderTypes.hpp>

namespace AN {

/// don't keep allocations, allocate every frame
struct UniformBufferAllocation {
    void    *buffer; // graphic backend specific buffer pointer
    UInt64   offset;
    UInt64   size;
    void    *mappedData;
};

class AN_API UniformBuffers {
public:

    virtual ~UniformBuffers() = default;

    virtual bool init() = 0;

    virtual void deinit() = 0;

    virtual void update() = 0;

};

AN_API UniformBuffers &GetUniformBuffers();


}

#endif//OJOIE_UNIFORMBUFFERS_HPP
