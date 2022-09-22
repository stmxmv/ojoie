//
// Created by aojoie on 9/20/2022.
//

#ifndef OJOIE_VK_PIPELINEREFLECTION_HPP
#define OJOIE_VK_PIPELINEREFLECTION_HPP

#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/ShaderLibrary.hpp"

namespace AN::VK {

class PipelineReflection {

public:
    bool reflectShaderResources(VkShaderStageFlagBits stage, const char *entryPoint,
                                const void *spirv, uint64_t size,
                                std::vector<ShaderResource> &resources);

};



}

#endif//OJOIE_VK_PIPELINEREFLECTION_HPP
