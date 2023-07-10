//
// Created by aojoie on 4/17/2023.
//

#include "Render/private/vulkan/ShaderFunction.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/ShaderFunction.hpp"

namespace AN::VK {

bool ShaderFunction::init(const UInt8 *code, size_t size) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode    = (const uint32_t *) code;

    if (VkResult result = vkCreateShaderModule(VK::GetDevice().vkDevice(), &createInfo, nullptr, &shaderModule);
        result != VK_SUCCESS) {
        AN_LOG(Error, "failed to create shader module, result %s", VK::ResultCString(result));
        shaderModule = nullptr;
        return false;
    }

    return true;
}

void ShaderFunction::deinit() {
    if (shaderModule) {
        vkDestroyShaderModule(VK::GetDevice().vkDevice(), shaderModule, nullptr);
        shaderModule = nullptr;
    }
}

}

