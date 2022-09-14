//
// Created by Aleudillonam on 9/6/2022.
//

#ifndef OJOIE_VK_RENDERCOMMANDENCODER_HPP
#define OJOIE_VK_RENDERCOMMANDENCODER_HPP

#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/CommandEncoder.hpp"
#include "Render/private/vulkan/Buffer.hpp"
#include "Render/RenderPipeline.hpp"
#include "Render/Sampler.hpp"
#include "Render/Texture.hpp"
#include "Render/IndexBuffer.hpp"
#include "Render/VertexBuffer.hpp"
#include "FrameBuffer.hpp"
#include "Layer.hpp"
#include "RenderPass.hpp"
#include "Device.hpp"


namespace AN::VK {



inline static VkShaderStageFlags toVkPipelineStageFlags(RC::ShaderStageFlag stageFlag) {
    VkShaderStageFlags ret{};
    if ((stageFlag & RC::ShaderStageFlag::Vertex) != RC::ShaderStageFlag::None) {
        ret |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if ((stageFlag & RC::ShaderStageFlag::Fragment) != RC::ShaderStageFlag::None) {
        ret |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if ((stageFlag & RC::ShaderStageFlag::Geometry) != RC::ShaderStageFlag::None) {
        ret |= VK_SHADER_STAGE_GEOMETRY_BIT;
    }
    return ret;
}

inline static VkCullModeFlags toVkCullMode(RC::CullMode cullMode) {
    VkCullModeFlags ret{};

    if ((cullMode & RC::CullMode::Front) != RC::CullMode::None) {
        ret |= VK_CULL_MODE_FRONT_BIT;
    }

    if ((cullMode & RC::CullMode::Back) != RC::CullMode::None) {
        ret |= VK_CULL_MODE_BACK_BIT;
    }

    return ret;
}

class RenderCommandEncoder : public CommandEncoder {

    RenderPass *renderPass;

    FrameBuffer *frameBuffer;

    const RenderTarget *_renderTarget;

    DescriptorSetManager *_descriptorSetManager;

    RC::RenderPipeline *currentPipeline{};

    VK::DescriptorSetInfo descriptorSetInfo;
    std::map<uint32_t, uint32_t> uniformBuffersOffsets;
    std::vector<uint32_t> descriptorSetDynamicOffsets;

public:

    RenderCommandEncoder(Device &device,
                         VkCommandBuffer commandBuffer,
                         const RenderTarget &renderTarget,
                         const RenderPassDescriptor &renderPassDescriptor,
                         DescriptorSetManager &descriptorSetManager)
        : CommandEncoder(commandBuffer), _renderTarget(&renderTarget), _descriptorSetManager(&descriptorSetManager) {

        renderPass = &device.getRenderResourceCache().newRenderPass(renderPassDescriptor);

        frameBuffer = &device.getRenderResourceCache().newFrameBuffer(renderTarget, *renderPass);

    }

    RenderPass &getRenderPass() const {
        return *renderPass;
    }

    template<typename ClearValues>
    void beginRenderPass(ClearValues &&clearValues) {
        // Begin render pass
        VkRenderPassBeginInfo begin_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        begin_info.renderPass        = renderPass->vkRenderPass();
        begin_info.framebuffer       = frameBuffer->vkFramebuffer();
        begin_info.renderArea.extent = _renderTarget->extent;
        begin_info.clearValueCount   = (uint32_t)(std::size(clearValues));
        begin_info.pClearValues      = std::data(clearValues);

        vkCmdBeginRenderPass(_commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }


    void nextSubPass() {
        vkCmdNextSubpass(_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    }

    /// \brief end render encoding, therefore end renderPass
    void endRenderPass() {
        vkCmdEndRenderPass(_commandBuffer);
    }

    void setViewport(const RC::Viewport &viewport) {
        VkViewport vkViewport;
        vkViewport.x = viewport.originX;
        vkViewport.y = viewport.originY;
        vkViewport.width = viewport.width;
        vkViewport.height = viewport.height;
        vkViewport.minDepth = viewport.znear;
        vkViewport.maxDepth = viewport.zfar;
        vkCmdSetViewport(_commandBuffer, 0, 1, &vkViewport);
    }

    void setScissor(const RC::ScissorRect &scissorRect) {
        VkRect2D rect;
        rect.offset.x = scissorRect.x;
        rect.offset.y = scissorRect.y;
        rect.extent.width = scissorRect.width;
        rect.extent.height = scissorRect.height;
        vkCmdSetScissor(_commandBuffer, 0, 1, &rect);
    }

    void setCullMode(RC::CullMode cullMode) {
        vkCmdSetCullMode(_commandBuffer, toVkCullMode(cullMode));
    }

    void bindRenderPipeline(class RC::RenderPipeline &renderPipeline) {
        if (currentPipeline != &renderPipeline) {
            currentPipeline = &renderPipeline;
            descriptorSetInfo.clear();
            uniformBuffersOffsets.clear();
            VkDescriptorSetLayout descriptorSetLayout = (VkDescriptorSetLayout) currentPipeline->getVkDescriptorLayout();
            descriptorSetInfo.layout = descriptorSetLayout;
            vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)renderPipeline.getVkPipeline());
        }
    }

    void bindUniformBuffer(uint32_t binding, uint64_t offset, uint64_t size, class VK::Buffer &uniformBuffer) {
        VkDescriptorBufferInfo &bufferInfo = descriptorSetInfo.bufferInfos[binding];

        bufferInfo.offset = 0;
        bufferInfo.buffer = uniformBuffer.vkBuffer();
        bufferInfo.range = size;

        descriptorSetInfo.descriptorTypes[binding] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

        uniformBuffersOffsets[binding] = offset;
    }

    void bindImageView(uint32_t binding, class VK::ImageView &imageView) {
        VkDescriptorImageInfo &imageInfo = descriptorSetInfo.imageInfos[binding];
        imageInfo.imageView = imageView.vkImageView();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = nullptr;

        descriptorSetInfo.descriptorTypes[binding] = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }

    void bindSampler(uint32_t binding, class RC::Sampler &sampler) {
        VkDescriptorImageInfo &imageInfo = descriptorSetInfo.imageInfos[binding];
        imageInfo.sampler = (VkSampler)sampler.getUnderlyingSampler();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.imageView = nullptr;

        descriptorSetInfo.descriptorTypes[binding] = VK_DESCRIPTOR_TYPE_SAMPLER;
    }

    /// \param offset element offset
    void bindIndexBuffer(RC::IndexType type, uint64_t offset, VK::Buffer &indexBuffer) {
        VkIndexType vkIndexType;
        if (type == RC::IndexType::UInt32) {
            vkIndexType = VK_INDEX_TYPE_UINT32;
        } else {
            // UInt16
            vkIndexType = VK_INDEX_TYPE_UINT16;
        }
        vkCmdBindIndexBuffer(_commandBuffer, indexBuffer.vkBuffer(), offset, vkIndexType);
    }

    void bindVertexBuffer(uint32_t binding, uint64_t offset, VK::Buffer &vertexBuffer) {
        VkBuffer vkBuffer = vertexBuffer.vkBuffer();
        vkCmdBindVertexBuffers(_commandBuffer, binding, 1, &vkBuffer, &offset);
    }

    void bindVertexBuffer(uint32_t binding, uint32_t bindingCount, const uint64_t *offset, VK::Buffer *const *vertexBuffer) {
        VkBuffer *buffers = (VkBuffer *)alloca(bindingCount * sizeof(VkBuffer));
        for (uint32_t i = 0; i < bindingCount; ++i) {
            buffers[i] = vertexBuffer[i]->vkBuffer();
        }
        vkCmdBindVertexBuffers(_commandBuffer, binding, bindingCount, buffers, offset);
    }

    void pushConstants(RC::ShaderStageFlag stageFlag, uint32_t offset, uint32_t size, const void *data) {
        vkCmdPushConstants(
                _commandBuffer,
                (VkPipelineLayout)currentPipeline->getVkPipelineLayout(),
                toVkPipelineStageFlags(stageFlag),
                offset, size, data
        );
    }

    void drawIndexed(uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0, uint32_t instanceCount = 1, uint32_t instanceOffset = 0) {
        descriptorSetDynamicOffsets.clear();
        descriptorSetDynamicOffsets.reserve(uniformBuffersOffsets.size());

        for (auto &[_, offset] : uniformBuffersOffsets) {
            descriptorSetDynamicOffsets.push_back(offset);
        }


        VkDescriptorSet descriptorSet = _descriptorSetManager->descriptorSet(descriptorSetInfo);

        VkPipelineLayout pipelineLayout = (VkPipelineLayout) currentPipeline->getVkPipelineLayout();

        vkCmdBindDescriptorSets(_commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout,
                                0, 1, &descriptorSet,
                                descriptorSetDynamicOffsets.size(), descriptorSetDynamicOffsets.data());


        vkCmdDrawIndexed(_commandBuffer, indexCount, instanceCount, indexOffset, (int32_t)vertexOffset, instanceOffset);
    }

    void draw(uint32_t count) {
        descriptorSetDynamicOffsets.clear();
        descriptorSetDynamicOffsets.reserve(uniformBuffersOffsets.size());

        for (auto &[_, offset] : uniformBuffersOffsets) {
            descriptorSetDynamicOffsets.push_back(offset);
        }


        VkDescriptorSet descriptorSet = _descriptorSetManager->descriptorSet(descriptorSetInfo);

        VkPipelineLayout pipelineLayout = (VkPipelineLayout) currentPipeline->getVkPipelineLayout();

        vkCmdBindDescriptorSets(_commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout,
                                0, 1, &descriptorSet,
                                descriptorSetDynamicOffsets.size(), descriptorSetDynamicOffsets.data());

        vkCmdDraw(_commandBuffer, count, 1, 0, 0);
    }
};

}

#endif//OJOIE_VK_RENDERCOMMANDENCODER_HPP
