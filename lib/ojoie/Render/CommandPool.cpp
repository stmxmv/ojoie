//
// Created by aojoie on 4/19/2023.
//

#include "Render/CommandPool.hpp"
#include "Render/RenderContext.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/CommandPool.hpp"
#endif//OJOIE_USE_VULKAN
#include "Render/private/D3D11/CommandBuffer.hpp"

namespace AN {

CommandPool *CommandPool::newCommandPool() {
    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        VK::CommandPool *pool = new VK::CommandPool();
        ANAssert(pool->init(VK::GetDevice()));
        return pool;
#endif//OJOIE_USE_VULKAN
    } else if (GetGraphicsAPI() == kGraphicsAPID3D11) {
        D3D11::CommandPool *pool = new D3D11::CommandPool();
        return pool;
    }
    return nullptr;
}

CommandPool &GetCommandPool() {
    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        return VK::GetCommandPool();
#endif//OJOIE_USE_VULKAN
    } else {
        return D3D11::GetCommandPool();
    }
    return D3D11::GetCommandPool();
}


}