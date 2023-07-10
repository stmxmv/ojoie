//
// Created by aojoie on 4/19/2023.
//

#include "Render/RenderContext.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/Instance.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/RenderResourceCache.hpp"
#include "Render/private/vulkan/CommandPool.hpp"
#include "Render/private/vulkan/DescriptorSetManager.hpp"
#include "Render/private/vulkan/TextureManager.hpp"
#endif //OJOIE_USE_VULKAN

#include "Render/private/D3D11/Device.hpp"
#include "Render/private/D3D11/TextureManager.hpp"
#include "Render/private/D3D11/UniformBuffers.hpp"
#include "Render/private/D3D11/VertexInputLayouts.hpp"
#include "Render/private/D3D11/CommandBuffer.hpp"

#include "Render/UniformBuffers.hpp"
#include "Render/RenderQueue.hpp"
#include "Threads/Dispatch.hpp"
#include "Threads/Task.hpp"

namespace AN {


static GraphicsAPI gAPI;

void InitializeRenderContext(GraphicsAPI api) {
    gAPI = api;
    if (api == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        VK::InitializeInstance();
        VK::InitializeDevice();
        ANAssert(VK::GetRenderResourceCache().init(VK::GetDevice()));
        VK::InitializeThreadCommandPool();
        ANAssert(VK::GetDescriptorSetManager().init(VK::GetDevice().vkDevice()));
#endif //OJOIE_USE_VULKAN
    } else if (api == kGraphicsAPID3D11) {
        D3D11::InitializeDevice();
    }
    ANAssert(GetUniformBuffers().init());
}

void DeallocRenderContext() {
    GetUniformBuffers().deinit();

    if (gAPI == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        VK::GetTextureManager().deallocTextureResources();
        VK::GetDescriptorSetManager().deinit();
        VK::DeallocThreadCommandPool();
        VK::GetRenderResourceCache().deinit();
        VK::DeallocDevice();
        VK::DeallocInstance();
#endif //OJOIE_USE_VULKAN
    } else if (gAPI == kGraphicsAPID3D11) {
        D3D11::GetCommandPool().deinit();
        D3D11::GetVertexInputLayouts().cleanup();
        D3D11::GetTextureManager().deallocTextureResources();
        D3D11::DestroyDevice();
    }
}

GraphicsAPI GetGraphicsAPI() {
    return gAPI;
}

void RenderContextWaitIdle() {
    if (gAPI == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        if (GetCurrentThreadID() != Dispatch::GetThreadID(Dispatch::Render) && Dispatch::IsRunning(Dispatch::Render)) {
            TaskFence taskFence;
            GetRenderQueue().enqueue([&] {
                VK::GetDevice().waitIdle();
                taskFence.signal();
            });
            taskFence.wait();
        } else {
            VK::GetDevice().waitIdle();
        }
#endif //OJOIE_USE_VULKAN
    }
}

}