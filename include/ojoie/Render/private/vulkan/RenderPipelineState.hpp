//
// Created by aojoie on 9/20/2022.
//

#ifndef OJOIE_VK_RENDERPIPELINESTATE_HPP
#define OJOIE_VK_RENDERPIPELINESTATE_HPP

#include "Render/RenderPipelineState.hpp"

#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {

class RenderPipeline;
class RenderPass;

class RenderPipelineState {
    RenderPipeline *_renderPipeline{};
    RenderPass *_renderPass{};
    RC::RenderPipelineStateDescriptor renderPipelineStateDescriptor;

    uint32_t _subpassIndex{};
public:

    bool init(const RC::RenderPipelineStateDescriptor &descriptor) {
        renderPipelineStateDescriptor = descriptor;
        return true;
    }

    void deinit() {}

    void flush() {
        _renderPipeline = nullptr;
        _renderPass = nullptr;
    }

    RenderPipeline &getRenderPipeline(RenderPass &renderPass, uint32_t subpassIndex) {
        if (_renderPipeline == nullptr || _renderPass->vkRenderPass() != renderPass.vkRenderPass() ||
            _subpassIndex != subpassIndex) {
            renderPipelineStateDescriptor.subpassIndex = subpassIndex;
            _renderPipeline = &renderPass.getDevice().getRenderResourceCache()
                                       .newRenderPipeline(renderPipelineStateDescriptor, renderPass);
            _subpassIndex = subpassIndex;
            _renderPass = &renderPass;
        }
        return *_renderPipeline;
    }


};


}

#endif//OJOIE_VK_RENDERPIPELINESTATE_HPP
