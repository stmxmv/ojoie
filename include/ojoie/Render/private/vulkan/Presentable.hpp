//
// Created by aojoie on 4/21/2023.
//

#ifndef OJOIE_VK_PRESENTABLE_HPP
#define OJOIE_VK_PRESENTABLE_HPP

#include <ojoie/Render/private/vulkan.hpp>
#include <ojoie/Render/RenderTarget.hpp>
#include <ojoie/Render/Layer.hpp>
#include <ojoie/BaseClasses/ObjectPtr.hpp>

namespace AN::VK {


struct Presentable : public AN::Presentable {
    VkSemaphore          signalSemaphore, acquireSemaphore;
    VkPipelineStageFlags waitStageFlag;
    VkSwapchainKHR       swapchain;
    uint32_t             imageIndex;

    ObjectPtr<AN::RenderTarget> _renderTarget;

    virtual UInt32 getFrameIndex() override {
        return imageIndex;
    }

    virtual AN::RenderTarget *getRenderTarget() override {
        return _renderTarget.get();
    }

};


}

#endif//OJOIE_VK_PRESENTABLE_HPP
