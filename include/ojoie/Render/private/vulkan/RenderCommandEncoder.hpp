//
// Created by Aleudillonam on 9/6/2022.
//

#ifndef OJOIE_VK_RENDERCOMMANDENCODER_HPP
#define OJOIE_VK_RENDERCOMMANDENCODER_HPP

#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/CommandEncoder.hpp"
#include "Render/private/vulkan/Buffer.hpp"
#include "Render/private/vulkan/Image.hpp"
#include "Render/private/vulkan/ResourceBindingState.hpp"
#include "Render/private/vulkan/RenderPipelineState.hpp"
#include "Render/Sampler.hpp"
#include "Render/Texture.hpp"
#include "Render/IndexBuffer.hpp"
#include "Render/VertexBuffer.hpp"
#include "FrameBuffer.hpp"
#include "Layer.hpp"
#include "RenderPass.hpp"
#include "Device.hpp"


namespace AN::VK {

#error "error"

inline static VkShaderStageFlags toVkPipelineStageFlags(ShaderStageFlags stageFlag) {
    VkShaderStageFlags ret{};
    if (stageFlag & kShaderStageVertex) {
        ret |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (stageFlag & kShaderStageFragment) {
        ret |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if (stageFlag & kShaderStageGeometry) {
        ret |= VK_SHADER_STAGE_GEOMETRY_BIT;
    }
    return ret;
}

inline static VkCullModeFlags toVkCullMode(CullMode cullMode) {
    VkCullModeFlags ret{};

    if (cullMode & kCullModeFront) {
        ret |= VK_CULL_MODE_FRONT_BIT;
    }

    if (cullMode & kCullModeBack) {
        ret |= VK_CULL_MODE_BACK_BIT;
    }

    return ret;
}

inline static bool is_dynamic_buffer_descriptor_type(VkDescriptorType descriptor_type) {
    return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
           descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
}

inline static bool is_buffer_descriptor_type(VkDescriptorType descriptor_type) {
    return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
           descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
           is_dynamic_buffer_descriptor_type(descriptor_type);
}

class RenderCommandEncoder : public CommandEncoder {
    Device *_device;
    RenderPass *renderPass;

    FrameBuffer *frameBuffer;

    DescriptorSetManager *_descriptorSetManager;

    RenderPipelineState *currentPipelineState{};
    RenderPipeline *currentPipeline{};

    std::vector<uint8_t> stored_push_constants;

    std::unordered_set<uint32_t> update_descriptor_sets;
    std::vector<uint32_t> dynamic_offsets;

    ResourceBindingState resourceBindingState;
    std::unordered_map<uint32_t, DescriptorSetLayout *> descriptor_set_layout_binding_state;

    DescriptorSetInfo descriptorSetInfo;

    uint32_t subpassIndex;
public:

    RenderCommandEncoder(Device &device,
                         VkCommandBuffer commandBuffer,
                         DescriptorSetManager &descriptorSetManager)
        : CommandEncoder(commandBuffer), _descriptorSetManager(&descriptorSetManager) {

        _device = &device;
    }

    RenderPass &getRenderPass() const {
        return *renderPass;
    }

    template<typename ClearValues, typename ImageViews>
    void beginRenderPass(VkExtent2D extent, ImageViews &&attchments,
                         const RenderPassDescriptor &renderPassDescriptor, ClearValues &&clearValues) {
//        renderPass = &GetRenderResourceCache().newRenderPass(renderPassDescriptor);
        frameBuffer = &GetRenderResourceCache().newFrameBuffer(*renderPass, extent,
                                                                        std::forward<ImageViews>(attchments));

        // Begin render pass
        VkRenderPassBeginInfo begin_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        begin_info.renderPass        = renderPass->vkRenderPass();
        begin_info.framebuffer       = frameBuffer->vkFramebuffer();
        begin_info.renderArea.extent = extent;
        begin_info.clearValueCount   = (uint32_t)(std::size(clearValues));
        begin_info.pClearValues      = std::data(clearValues);

        vkCmdBeginRenderPass(_commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

        subpassIndex = 0;
    }


    void nextSubPass() {
        vkCmdNextSubpass(_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        subpassIndex += 1;
    }

    /// \brief end render encoding, therefore end renderPass
    void endRenderPass() {
        vkCmdEndRenderPass(_commandBuffer);
    }

    void setCullMode(CullMode cullMode) {
        vkCmdSetCullMode(_commandBuffer, toVkCullMode(cullMode));
    }

    void setRenderPipelineState(RenderPipelineState &renderPipelineState) {
        if (currentPipelineState != &renderPipelineState) {
            currentPipelineState = &renderPipelineState;

//            currentPipeline = &renderPipelineState.getRenderPipeline(*renderPass, subpassIndex);
            vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline->vkPipeline());
        }
    }

    void bindUniformBuffer(uint32_t binding, uint64_t offset, uint64_t size, class VK::Buffer &uniformBuffer
                           ,uint32_t set = 0, uint32_t arrayElement = 0) {
//        resourceBindingState.bind_buffer(uniformBuffer, offset, size, set, binding, arrayElement);
    }

    void bindImageView(uint32_t binding, const class VK::ImageView &imageView, uint32_t set = 0, uint32_t arrayElement = 0) {
//        resourceBindingState.bind_image(imageView, set, binding, arrayElement);
    }

    void bindSampler(uint32_t binding, class RC::Sampler &sampler, uint32_t set = 0, uint32_t arrayElement = 0) {
//        resourceBindingState.bind_sampler(sampler, set, binding, arrayElement);
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

    void pushConstants(uint32_t offset, uint32_t size, const void *data) {
        PipelineLayout &pipelineLayout = currentPipeline->getPipelineLayout();
        VkShaderStageFlags shader_stage = pipelineLayout.getPushConstantRangeStage(offset, size);
        if (shader_stage) {
            vkCmdPushConstants(
                    _commandBuffer,
                    pipelineLayout.vkPipelineLayout(),
                    shader_stage,
                    offset, size, data
            );
        } else {
            ANLog("Push constant range [%d, %d] not found", offset, size);
        }
    }

    void drawIndexed(uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0,
                     uint32_t instanceCount = 1, uint32_t instanceOffset = 0) {
        flushDescriptorState();
        vkCmdDrawIndexed(_commandBuffer, indexCount, instanceCount, indexOffset, (int32_t)vertexOffset, instanceOffset);
    }

    void draw(uint32_t count) {
        flushDescriptorState();
        vkCmdDraw(_commandBuffer, count, 1, 0, 0);
    }

private:
    void flushDescriptorState() {
        const auto &pipeline_layout = currentPipeline->getPipelineLayout();
//        const auto &shader_program = pipeline_layout.getShaderProgram();

        update_descriptor_sets.clear();

        // Iterate over the shader sets to check if they have already been bound
        // If they have, add the set so that the command buffer later updates it
        for (const auto &set_it : pipeline_layout.getDescriptorSetLayouts()) {
            uint32_t descriptor_set_id = set_it.first;
            auto descriptor_set_layout_it = descriptor_set_layout_binding_state.find(descriptor_set_id);
            if (descriptor_set_layout_it != descriptor_set_layout_binding_state.end()) {
                if (descriptor_set_layout_it->second->vkDescriptorSetLayout() !=
                    pipeline_layout.getDescriptorSetLayout(descriptor_set_id).vkDescriptorSetLayout()) {
                    update_descriptor_sets.emplace(descriptor_set_id);
                }
            }
        }

        // Validate that the bound descriptor set layouts exist in the pipeline layout
        for (auto set_it = descriptor_set_layout_binding_state.begin(); set_it != descriptor_set_layout_binding_state.end();) {
            if (!pipeline_layout.hasDescriptorSetLayout(set_it->first)) {
                set_it = descriptor_set_layout_binding_state.erase(set_it);
            } else {
                ++set_it;
            }
        }

        // Check if a descriptor set needs to be created
        if (resourceBindingState.is_dirty() || !update_descriptor_sets.empty()) {
            resourceBindingState.clear_dirty();

            // Iterate over all of the resource sets bound by the command buffer
            for (const auto &resource_set_it : resourceBindingState.get_resource_sets()) {
                uint32_t descriptor_set_id = resource_set_it.first;
                const auto &   resource_set      = resource_set_it.second;

                // Don't update resource set if it's not in the update list OR its state hasn't changed
                if (!resource_set.is_dirty() &&
                    (update_descriptor_sets.find(descriptor_set_id) == update_descriptor_sets.end())) {
                    continue;
                }

                // Clear dirty flag for resource set
                resourceBindingState.clear_dirty(descriptor_set_id);

                // Skip resource set if a descriptor set layout doesn't exist for it
                if (!pipeline_layout.hasDescriptorSetLayout(descriptor_set_id)) {
                    continue;
                }

                auto &descriptor_set_layout = pipeline_layout.getDescriptorSetLayout(descriptor_set_id);

                // Make descriptor set layout bound for current set
                descriptor_set_layout_binding_state[descriptor_set_id] = &descriptor_set_layout;

                descriptorSetInfo.reset();
                descriptorSetInfo.layout = &descriptor_set_layout;
                auto &buffer_infos = descriptorSetInfo.bufferInfos;
                auto &image_infos = descriptorSetInfo.imageInfos;

                dynamic_offsets.clear();

                // Iterate over all resource bindings
                for (const auto &binding_it : resource_set.get_resource_bindings()) {
                    auto binding_index      = binding_it.first;
                    const auto &binding_resources = binding_it.second;

                    // Check if binding exists in the pipeline layout
                    if (auto binding_info = descriptor_set_layout.getLayoutBinding(binding_index)) {
                        // Iterate over all binding resources
                        for (const auto &element_it : binding_resources) {
                            auto array_element  = element_it.first;
                            const auto &resource_info = element_it.second;

                            // Pointer references
                            const auto &buffer     = resource_info.buffer;
                            const auto &sampler    = resource_info.sampler;
                            const auto &image_view = resource_info.image_view;

                            // Get buffer info
                            if (buffer != nullptr && is_buffer_descriptor_type(binding_info->descriptorType)) {
                                VkDescriptorBufferInfo buffer_info{};

//                                buffer_info.buffer = resource_info.buffer->vkBuffer();
                                buffer_info.offset = resource_info.offset;
                                buffer_info.range  = resource_info.range;

                                if (is_dynamic_buffer_descriptor_type(binding_info->descriptorType)) {
                                    dynamic_offsets.push_back((uint32_t) (buffer_info.offset));
                                    buffer_info.offset = 0;
                                }

                                buffer_infos[binding_index][array_element] = buffer_info;
                            }

                            // Get image info
                            else if (image_view != nullptr || sampler != VK_NULL_HANDLE) {
                                // Can be null for input attachments
                                VkDescriptorImageInfo image_info{};
//                                image_info.sampler   = sampler ? (VkSampler) sampler->getUnderlyingSampler() : VK_NULL_HANDLE;
//                                image_info.imageView = image_view ? image_view->vkImageView() : VK_NULL_HANDLE;

                                if (image_view != nullptr) {
                                    // Add image layout info based on descriptor type
                                    switch (binding_info->descriptorType) {
                                        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                                        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                                        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
//                                            if (is_depth_stencil_format(image_view->getFormat())) {
//                                                image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
//                                            } else {
//                                                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//                                            }
                                            break;
                                        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                                            image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                                            break;

                                        default:
                                            break;
                                    }
                                }

                                image_infos[binding_index][array_element] = image_info;
                            }
                        }
                    }
                }




                VkDescriptorSet descriptorSet = _descriptorSetManager->descriptorSet(descriptorSetInfo);

                // Bind descriptor set
                vkCmdBindDescriptorSets(_commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipeline_layout.vkPipelineLayout(),
                                        descriptor_set_id,
                                        1, &descriptorSet,
                                        (uint32_t)(dynamic_offsets.size()),
                                        dynamic_offsets.data());
            }
        }

    }
};

}

#endif//OJOIE_VK_RENDERCOMMANDENCODER_HPP
