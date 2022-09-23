//
// Created by Aleudillonam on 9/9/2022.
//

#ifndef OJOIE_VK_RENDERPIPELINE_HPP
#define OJOIE_VK_RENDERPIPELINE_HPP

#include "Render/RenderPipelineState.hpp"
#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/PipelineLayout.hpp"

namespace AN::VK {

class Device;
class RenderPass;

class RenderPipeline : private NonCopyable {

    Device *_device;
    PipelineLayout *_pipelineLayout;
    VkPipeline graphicsPipeline{};

public:

    RenderPipeline() = default;

    RenderPipeline(RenderPipeline &&other) noexcept
        : _device(other._device), _pipelineLayout(other._pipelineLayout), graphicsPipeline(other.graphicsPipeline) {
        other.graphicsPipeline = nullptr;
    }

    ~RenderPipeline() {
        deinit();
    }

    bool init(Device &device, RC::RenderPipelineStateDescriptor &descriptor, RenderPass &renderPass,
              VkPipelineCache pipelineCache = nullptr);

    void deinit();

    VkPipeline vkPipeline() const {
        return graphicsPipeline;
    }

    PipelineLayout &getPipelineLayout() const {
        return *_pipelineLayout;
    }

};

}

#endif//OJOIE_VK_RENDERPIPELINE_HPP
