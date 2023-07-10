//
// Created by aojoie on 5/4/2023.
//

#include "Render/UniformBuffers.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/UniformBuffers.hpp"
#endif//OJOIE_USE_VULKAN

#include "Render/private/D3D11/UniformBuffers.hpp"

namespace AN {


UniformBuffers &GetUniformBuffers() {
    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        return VK::GetUniformBuffers();
#endif//OJOIE_USE_VULKAN
    } else {
        return D3D11::GetUniformBuffers();
    }
    return D3D11::GetUniformBuffers();
}


}