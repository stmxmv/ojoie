//
// Created by aojoie on 9/20/2022.
//

#ifndef OJOIE_VK_SHADERPROGRAM_HPP
#define OJOIE_VK_SHADERPROGRAM_HPP

#include "ojoie/Render/private/vulkan/ShaderLibrary.hpp"

namespace AN::VK {

//class ShaderProgram {
//    std::vector<ShaderFunction> functions;
//
//    // The shader resources that this program uses, indexed by their name
//    std::unordered_map<std::string, ShaderResource> resources;
//
//    // A map of each set and the resources it owns used by the shader program
//    std::unordered_map<uint32_t, std::vector<ShaderResource>> sets;
//
//    bool initCommon();
//
//public:
//
//    template<typename Functions>
//    bool init(Functions &&funcs) {
//        functions.assign(std::begin(funcs), std::end(funcs));
//        return initCommon();
//    }
//
//    bool init(ShaderFunction *funcs, uint64_t size) {
//        functions.assign(funcs, funcs + size);
//        return initCommon();
//    }
//
//    std::vector<ShaderResource> getResources(const ShaderResourceType &type = ShaderResourceType::All, VkShaderStageFlagBits stage = VK_SHADER_STAGE_ALL) const;
//
//    const std::unordered_map<uint32_t, std::vector<ShaderResource>> &getDescriptorSets() const {
//        return sets;
//    }
//};


}
#endif//OJOIE_VK_SHADERPROGRAM_HPP
