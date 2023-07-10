//
// Created by aojoie on 4/14/2023.
//

#ifndef OJOIE_SHADERINFO_HPP
#define OJOIE_SHADERINFO_HPP

#include <ojoie/Core/Name.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/ShaderLab/Property.hpp>

#include <unordered_map>
#include <vector>

namespace AN::ShaderLab {

typedef std::unordered_map<Name, Name> TagMap;

struct ShaderPass {
    TagMap tagMap;

    bool            ZWrite;
    CompareFunction ZTestOperation;

    bool           blending;
    BlendFactor    sourceBlendFactor;
    BlendFactor    destinationBlendFactor;
    BlendFactor    sourceAlphaFactor;
    BlendFactor    destinationAlphaFactor;
    BlendOperation blendOperation;

    ColorWriteMask colorWriteMask;

    CullMode cullMode;

    bool ZClip;

    inline static const char* GetTypeString() { return "ShaderLabPass"; }

    template<typename Coder>
    void transfer(Coder &coder) {
        TRANSFER(tagMap);
        TRANSFER(ZWrite);
        TRANSFER(ZTestOperation);
        TRANSFER(blending);
        TRANSFER(sourceBlendFactor);
        TRANSFER(destinationBlendFactor);
        TRANSFER(sourceAlphaFactor);
        TRANSFER(destinationAlphaFactor);
        TRANSFER(blendOperation);
        TRANSFER(colorWriteMask);
        TRANSFER(cullMode);
        TRANSFER(ZClip);
    }
};

struct SubShader {
    TagMap                  tagMap;
    std::vector<ShaderPass> passes;
};

struct ShaderInfo {

    Name                                  name;
    std::vector<Property>                 properties;
    std::vector<SubShader>                subShaders;
    std::vector<std::string>              subShaderHLSLIncludes;
    std::vector<std::vector<std::string>> passHLSLSources;
};


}// namespace AN::ShaderLab

#endif//OJOIE_SHADERINFO_HPP
