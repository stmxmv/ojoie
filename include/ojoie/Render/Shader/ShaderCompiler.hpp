//
// Created by aojoie on 4/13/2023.
//

#ifndef OJOIE_SHADERCOMPILER_HPP
#define OJOIE_SHADERCOMPILER_HPP

#include <ojoie/Render/RenderTypes.hpp>
#include <span>

namespace AN::RC {


class AN_API ShaderCompiler {

public:

    ShaderCompiler();

    std::vector<UInt8> compileHLSLToSPIRV(ShaderStage stage,
                                          const char *source,
                                          const char *entry,
                                          const char *nameHint = nullptr,
                                          std::span<const char *> include = {},
                                          bool emitSemantic = true);

    std::vector<UInt8> compileHLSLToCSO(ShaderStage stage,
                                          const char *source,
                                          const char *entry,
                                          const char *nameHint = nullptr,
                                          std::span<const char *> include = {},
                                          bool emitSemantic = true);
};


}

#endif//OJOIE_SHADERCOMPILER_HPP
