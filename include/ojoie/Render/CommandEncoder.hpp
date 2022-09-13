//
// Created by Aleudillonam on 9/13/2022.
//

#ifndef OJOIE_RC_COMMANDENCODER_HPP
#define OJOIE_RC_COMMANDENCODER_HPP

namespace AN::RC {

enum class PipelineStageFlag {
    None         = 0,
    VertexInput  = 1 << 0,
    VertexShader = 1 << 1,
    Transfer     = 1 << 2

};

enum class PipelineAccessFlag {
    None          = 0,
    TransferRead  = 1 << 0,
    TransferWrite = 1 << 1,
    ShaderRead    = 1 << 2,
    ShaderWrite   = 1 << 3
};

struct BufferMemoryBarrier {
    PipelineStageFlag srcStageFlag;
    PipelineStageFlag dstStageFlag;
    PipelineAccessFlag srcAccessMask;
    PipelineAccessFlag dstAccessMask;
    uint64_t offset;
    uint64_t size;
};

}

namespace AN {

template<>
struct enable_bitmask_operators<RC::PipelineStageFlag> : std::true_type {};

template<>
struct enable_bitmask_operators<RC::PipelineAccessFlag> : std::true_type {};

}

#endif//OJOIE_RC_COMMANDENCODER_HPP
