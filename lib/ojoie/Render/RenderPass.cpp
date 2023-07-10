//
// Created by aojoie on 4/21/2023.
//

#include "Render/RenderPass.hpp"
#include "Render/RenderContext.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/RenderPass.hpp"
#endif//OJOIE_USE_VULKAN
#include "Render/private/D3D11/RenderPass.hpp"


namespace AN {


bool RenderPass::init(const AN::RenderPassDescriptor &renderPassDescriptor) {
    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        impl = new VK::RenderPass();
#endif//OJOIE_USE_VULKAN
    } else if (GetGraphicsAPI() == kGraphicsAPID3D11) {
        impl = new D3D11::RenderPass();
    }

    return impl->init(renderPassDescriptor);
}

void RenderPass::deinit() {
    if (impl) {
        impl->deinit();
        delete impl;
        impl = nullptr;
    }
}

}