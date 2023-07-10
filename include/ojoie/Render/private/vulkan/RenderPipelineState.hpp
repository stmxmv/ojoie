//
// Created by aojoie on 9/20/2022.
//

#ifndef OJOIE_VK_RENDERPIPELINESTATE_HPP
#define OJOIE_VK_RENDERPIPELINESTATE_HPP

#include "Render/RenderPipelineState.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/ShaderFunction.hpp"
#include <ojoie/Render/VertexData.hpp>

namespace AN::VK {

class RenderPipeline;
class RenderPass;

class RenderPipelineState : public RenderPipelineStateImpl {


    VK::ShaderFunction _vertexFunction;
    VK::ShaderFunction _fragmentFunction;

    PipelineLayout *_pipelineLayout;
    RenderPass     *_renderPass{};

    RenderPipelineStateDescriptor renderPipelineStateDescriptor;

    /// pipeline map vertex input layout
    std::unordered_map<size_t, std::unique_ptr<RenderPipeline>> _renderPipelineVertexMap;

    uint32_t _subpassIndex{};

public:
    virtual bool init(const RenderPipelineStateDescriptor &renderPipelineDescriptor,
                      const AN::PipelineReflection        &reflection) override;

    virtual void deinit() override;

    void flush() {
        _renderPipelineVertexMap.clear();
        _renderPass = nullptr;
    }

    /// TODO provide an async version
    /// try to build pipeline, if pipeline exist just simply return, pipeline is owned by state object
    RenderPipeline *buildRenderPipeline(RenderPass &renderPass, uint32_t subpassIndex,
                                        const VertexDescriptor &vertexDescriptor);

//    virtual AN::RenderPipeline *buildRenderPipeline(AN::RenderPass &renderPass, uint32_t subpassIndex,
//                                                const VertexDescriptor &vertexDescriptor) override {
//        return (AN::RenderPipeline *)buildRenderPipeline(*(RenderPass *)renderPass.getImpl(),
//                                                          subpassIndex,
//                                                          vertexDescriptor);
//    }
};


}// namespace AN::VK

#endif//OJOIE_VK_RENDERPIPELINESTATE_HPP
