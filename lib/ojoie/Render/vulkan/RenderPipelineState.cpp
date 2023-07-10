//
// Created by aojoie on 9/20/2022.
//

#include "Render/RenderPipelineState.hpp"
#include "Render/private/vulkan/RenderPipelineState.hpp"

namespace AN::VK {

bool RenderPipelineState::init(const RenderPipelineStateDescriptor &renderPipelineDescriptor,
                               const AN::PipelineReflection        &reflection) {
    renderPipelineStateDescriptor = renderPipelineDescriptor;

    if (!_vertexFunction.init(renderPipelineDescriptor.vertexFunction.code,
                              renderPipelineDescriptor.vertexFunction.size)) {
        return false;
    }

    if (!_fragmentFunction.init(renderPipelineDescriptor.fragmentFunction.code,
                                renderPipelineDescriptor.fragmentFunction.size)) {
        return false;
    }

    //// RenderPipeline init will use code as VkShaderModule
    renderPipelineStateDescriptor.vertexFunction.code   = (const UInt8 *) _vertexFunction.getVkShaderModule();
    renderPipelineStateDescriptor.fragmentFunction.code = (const UInt8 *) _fragmentFunction.getVkShaderModule();

    /// default use vulkan dynamic offset descriptor
    _pipelineLayout = &GetRenderResourceCache().newPipelineLayout(reflection.getResources(), true);

    return true;
}

void RenderPipelineState::deinit() {
    flush();
    _vertexFunction.deinit();
    _fragmentFunction.deinit();
    _renderPipelineVertexMap.clear();
}

RenderPipeline *RenderPipelineState::buildRenderPipeline(RenderPass &renderPass, uint32_t subpassIndex,
                                                       const VertexDescriptor &vertexDescriptor) {

    size_t layoutHashValue = std::hash<VertexDescriptor>()(vertexDescriptor);

    auto it = _renderPipelineVertexMap.find(layoutHashValue);

    RenderPipeline *renderPipelinePtr;

    if (_renderPass != &renderPass ||
        _subpassIndex != subpassIndex ||
        it == _renderPipelineVertexMap.end()) {


        _subpassIndex = subpassIndex;
        _renderPass   = &renderPass;

        VK::RenderPipelineDescriptor pipelineDescriptor{};
        pipelineDescriptor.stateDescriptor = &renderPipelineStateDescriptor;
        pipelineDescriptor.subpassIndex    = subpassIndex;
        pipelineDescriptor.vertexDescriptor = &vertexDescriptor;


        auto renderPipeline = std::make_unique<RenderPipeline>();
        renderPipelinePtr = renderPipeline.get();

        ANAssert(renderPipeline->init(GetDevice(),
                                      pipelineDescriptor,
                                      *_pipelineLayout,
                                      *_renderPass,
                                      VK::GetRenderResourceCache().getPipelineCache()));

        _renderPipelineVertexMap.insert({ layoutHashValue, std::move(renderPipeline) });

    } else {
        renderPipelinePtr = it->second.get();
    }

    return renderPipelinePtr;
}


}// namespace AN::VK
