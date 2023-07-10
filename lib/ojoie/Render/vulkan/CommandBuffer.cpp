//
// Created by Aleudillonam on 9/10/2022.
//

#include "Render/private/vulkan/CommandBuffer.hpp"
#include "Render/private/vulkan/SemaphorePool.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Threads/Dispatch.hpp"
#include "Render/CommandBuffer.hpp"
#include "Render/private/vulkan/RenderPass.hpp"
#include "Render/private/vulkan/RenderTypes.hpp"
#include "Render/private/vulkan/RenderPipelineState.hpp"

#include "Render/private/vulkan/TextureManager.hpp"

namespace AN::VK {

namespace {

inline static bool is_dynamic_buffer_descriptor_type(VkDescriptorType descriptor_type) {
    return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
           descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
}

inline static bool is_buffer_descriptor_type(VkDescriptorType descriptor_type) {
    return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
           descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
           is_dynamic_buffer_descriptor_type(descriptor_type);
}

}

VkPipelineStageFlags toVkRenderType(PipelineStageFlagBits stageFlag) {

    if (stageFlag == kPipelineStageNone) return VK_PIPELINE_STAGE_NONE;

    VkPipelineStageFlags ret{};
    if (stageFlag & kPipelineStageVertexInput) {
        ret |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    }
    if (stageFlag & kPipelineStageVertexShader) {
        ret |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    }
    if (stageFlag & kPipelineStageTransfer) {
        ret |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    if (stageFlag & kPipelineStageColorAttachmentOutput) {
        ret |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    if (stageFlag & kPipelineStageTopOfPipe) {
        ret |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
    if (stageFlag & kPipelineStageEarlyFragmentTests) {
        ret |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    if (stageFlag & kPipelineStageLateFragmentTests) {
        ret |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    if (stageFlag & kPipelineStageBottomOfPipe) {
        ret |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }

    return ret;
}

VkAccessFlags toVkRenderType(AccessFlagBits flag) {
    if (flag == kAccessNone) return VK_ACCESS_NONE;

    VkAccessFlags ret{};
    if (flag & kAccessTransferRead) {
        ret |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if (flag & kAccessTransferWrite) {
        ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if (flag & kAccessShaderRead) {
        ret |= VK_ACCESS_SHADER_READ_BIT;
    }
    if (flag & kAccessShaderWrite) {
        ret |= VK_ACCESS_SHADER_WRITE_BIT;
    }
    if (flag & kAccessColorAttachmentWrite) {
        ret |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (flag & kAccessDepthStencilAttachmentRead) {
        ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    }
    if (flag & kAccessDepthStencilAttachmentWrite) {
        ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    return ret;
}

VkImageAspectFlags toVkRenderType(TextureAspectFlagBits flag) {
    if (flag == kTextureAspectNone) return VK_IMAGE_ASPECT_NONE;

    VkImageAspectFlags ret{};
    if (flag & kTextureAspectColor) {
        ret |= VK_IMAGE_ASPECT_COLOR_BIT;
    }
    if (flag & kTextureAspectDepth) {
        ret |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (flag & TextureAspectStencil) {
        ret |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    return ret;
}

VkImageLayout toVkRenderType(TextureLayout layout) {
    switch (layout) {
        case kTextureLayoutColorAttachmentOptimal:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case kTextureLayoutPresentSrc:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        case kTextureLayoutUndefined:
            return VK_IMAGE_LAYOUT_UNDEFINED;
        case kTextureLayoutDepthStencilAttachmentOptimal:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case kTextureLayoutTransferSrcOptimal:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case kTextureLayoutTransferDstOptimal:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }
    throw AN::Exception("invalid TextureLayout enum value");
}

VkFilter toVkRenderType(BlitFilter filter) {
    switch (filter) {
        case kBlitFilterNearest:
            return VK_FILTER_NEAREST;
        case kBlitFilterLinear:
            return VK_FILTER_LINEAR;
    }
    throw AN::Exception("invalid BlitFilter enum value");
}

void CommandBuffer::submit(const Queue &queue) {
    if (_queueFamilyIndex != queue.getFamilyIndex()) {
        AN_LOG(Error, "vulkan queue family index is %d, but submitted commandBuffer family index is %d",
               queue.getFamilyIndex(), _queueFamilyIndex);
        return;
    }

    /// add our semaphore, so host can wait until complete in any threads
    /// I think no need to consider semaphore will exceed UInt64, cause
    /// if we have 200 FPS, if will have decades
    addSignalSemaphore(_semaphore.vkSemaphore(), lastSemaphoreValue + 1);
    lastSemaphoreValue += 1;

    if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
        ANLog("failed to end recording command buffer!");
        return;
    }

    VkSubmitInfo submit_info[1] = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

    VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo{ VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    timelineSemaphoreSubmitInfo.signalSemaphoreValueCount = signalSemaphoreValues.size();
    timelineSemaphoreSubmitInfo.pSignalSemaphoreValues = signalSemaphoreValues.data();
    timelineSemaphoreSubmitInfo.waitSemaphoreValueCount = waitSemaphoreValues.size();
    timelineSemaphoreSubmitInfo.pWaitSemaphoreValues = waitSemaphoreValues.data();

    submit_info[0].pNext = &timelineSemaphoreSubmitInfo;

    submit_info[0].commandBufferCount   = 1;
    submit_info[0].pCommandBuffers      = &_commandBuffer;

    submit_info[0].waitSemaphoreCount   = waitSemaphores.size();
    submit_info[0].pWaitSemaphores      = waitSemaphores.data();
    submit_info[0].pWaitDstStageMask    = waitDstStageFlags.data();

    submit_info[0].signalSemaphoreCount = signalSemaphores.size();
    submit_info[0].pSignalSemaphores    = signalSemaphores.data();

    VkResult result = queue.submit(submit_info, _fence);

    if (result != VK_SUCCESS) {
        AN_LOG(Error, "commandBuffer submit error : %s", ResultCString(result));
//        return;
    }

    if (!swapchains.empty()) {
        VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present_info.waitSemaphoreCount = presentWaitSemaphores.size();
        present_info.pWaitSemaphores    = presentWaitSemaphores.data(); // wait until render command complete
        present_info.swapchainCount     = swapchains.size();
        present_info.pSwapchains        = swapchains.data();
        present_info.pImageIndices      = imageIndices.data();
        queue.present(present_info); /// we just ignore the present result, since acquire next image will return the same result
    }
}

void CommandBuffer::submit() {

    if (GetCurrentThreadID() == Dispatch::GetThreadID(Dispatch::Render) || !Dispatch::IsRunning(Dispatch::Render)) {
        submit(GetDevice().getGraphicsQueue());
    } else {
        TaskFence fence;
        /// commandBuffer is guaranteed to be completed after submit, this pointer is valid
        Dispatch::async(Dispatch::Render, [this, &fence] {
            submit(GetDevice().getGraphicsQueue());
            fence.signal();
        });
        fence.wait();
    }
}

CommandBuffer::CommandBuffer(Device &device, uint32_t queueFamilyIndex,
                             VkCommandBuffer commandBuffer,
                             CommandBufferResetMode resetMode,
                             VkFence fence)
        : _device(&device),
          _queueFamilyIndex(queueFamilyIndex),
          _commandBuffer(commandBuffer),
          _resetMode(resetMode),
          lastSemaphoreValue(),
          _fence(fence) {

    /// init semaphore
    ANAssert(_semaphore.init(false));

}

void CommandBuffer::beginRecord() {
    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional
    vkBeginCommandBuffer(_commandBuffer, &beginInfo);
}

void CommandBuffer::present(const AN::Presentable &_presentable) {
    VK::Presentable &presentable = (VK::Presentable &)_presentable;
    swapchains.push_back(presentable.swapchain);
    imageIndices.push_back(presentable.imageIndex);

    addWaitSemaphore(presentable.acquireSemaphore, presentable.waitStageFlag); // wait until next image acquired
    addSignalSemaphore(presentable.signalSemaphore);

    /// submit to present queue will wait this sema
    presentWaitSemaphores.push_back(presentable.signalSemaphore);
}

void CommandBuffer::waitUntilCompleted() {
    _semaphore.wait(lastSemaphoreValue);
//    vkWaitForFences(_device->vkDevice(), 1, &_fence, VK_TRUE, UINT_MAX);
}

bool CommandBuffer::isCompleted() {
    return _semaphore.getValue() == lastSemaphoreValue;
//    VkResult result = vkGetFenceStatus(_device->vkDevice(), _fence);
//    return result == VK_SUCCESS; /// The fence specified by fence is signaled.
}

void CommandBuffer::reset() {
    swapchains.clear();
    imageIndices.clear();
    waitSemaphores.clear();
    waitDstStageFlags.clear();
    signalSemaphores.clear();

    signalSemaphoreValues.clear();
    waitSemaphoreValues.clear();

    presentWaitSemaphores.clear();

    _renderPass = nullptr;
    currentPipeline = nullptr;
    stored_push_constants.clear();
    update_descriptor_sets.clear();
    dynamic_offsets.clear();
    resourceBindingState.reset();
    descriptor_set_layout_binding_state.clear();
    descriptorSetInfo.reset();

    _vertexDescriptor.attributes.clear();
    _vertexDescriptor.layouts.clear();

    // if resetMode is resetPool don't need to reset fence cause pool will reset all fences


    if (_resetMode == kCommandBufferResetModeResetIndividually) {
        vkResetFences(GetDevice().vkDevice(), 1, &_fence);
        vkResetCommandBuffer(_commandBuffer, 0);
    }

}

void CommandBuffer::debugLabelBegin(const char *name, Vector4f color) {
    if (vkCmdBeginDebugUtilsLabelEXT) {
        VkDebugUtilsLabelEXT label{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
        label.pLabelName = name;
        label.color[0] = color.r;
        label.color[1] = color.g;
        label.color[2] = color.b;
        label.color[3] = color.a;
        vkCmdBeginDebugUtilsLabelEXT(_commandBuffer, &label);
    }
}

void CommandBuffer::debugLabelEnd() {
    if (vkCmdEndDebugUtilsLabelEXT) {
        vkCmdEndDebugUtilsLabelEXT(_commandBuffer);
    }
}

void CommandBuffer::debugLabelInsert(const char *name, Vector4f color) {
    if (vkCmdInsertDebugUtilsLabelEXT) {
        VkDebugUtilsLabelEXT label{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
        label.pLabelName = name;
        label.color[0] = color.r;
        label.color[1] = color.g;
        label.color[2] = color.b;
        label.color[3] = color.a;
        vkCmdInsertDebugUtilsLabelEXT(_commandBuffer, &label);
    }
}

void CommandBuffer::copyBufferToImage(VkBuffer buffer, uint64_t bufferOffset, uint64_t bufferRowLength, uint64_t bufferImageHeight, VkImage image, uint32_t mipmapLevel, int32_t xOffset, int32_t yOffset, uint32_t width, uint32_t height, VkImageAspectFlags aspectMask, uint32_t layerCount) {

    VkBufferImageCopy region{};
    region.bufferOffset = bufferOffset;
    region.bufferRowLength = bufferRowLength;
    region.bufferImageHeight = bufferImageHeight;
    region.imageSubresource.aspectMask = aspectMask;
    region.imageSubresource.mipLevel = mipmapLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;
    region.imageOffset = { .x = xOffset, .y = yOffset };
    region.imageExtent = {
            width,
            height,
            1
    };

    vkCmdCopyBufferToImage(_commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CommandBuffer::imageBarrier(VkPipelineStageFlags srcMask, VkPipelineStageFlags dstMask,
                                 const VkImageMemoryBarrier &memoryBarrier) {

    vkCmdPipelineBarrier(
            _commandBuffer,
            srcMask,
            dstMask,
            0,
            0, nullptr,
            0, nullptr,
            1,
            &memoryBarrier);
}

void CommandBuffer::generateMipmaps(VkImage image, UInt32 width, UInt32 height, uint32_t mipLevels) {

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = (int32_t)width;
    int32_t mipHeight = (int32_t)height;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(_commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        barrier.subresourceRange.baseMipLevel = i;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(_commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(_commandBuffer,
                       image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(_commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        if (mipWidth > 1) {
            mipWidth /= 2;
        }
        if (mipHeight > 1) {
            mipHeight /= 2;
        }
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(_commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

}

void CommandBuffer::bufferBarrier(VkPipelineStageFlags srcMask, VkPipelineStageFlags dstMask,
                                  const VkBufferMemoryBarrier &bufferMemoryBarrier) {
    vkCmdPipelineBarrier(_commandBuffer,
                         srcMask,
                         dstMask,
                         0,
                         0, nullptr,
                         1, &bufferMemoryBarrier,
                         0, nullptr);
}

void CommandBuffer::beginRenderPass(UInt32 width, UInt32 height,
                                    AN::RenderPass                     &renderPass,
                                    std::span<const AN::RenderTarget *> renderTargets,
                                    std::span<ClearValue>               clearValues) {
    _renderPass = (VK::RenderPass *) renderPass.getImpl();
    // Begin render pass
    VkRenderPassBeginInfo begin_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin_info.renderPass               = _renderPass->vkRenderPass();
    begin_info.framebuffer              = _renderPass->getFramebuffer();
    begin_info.renderArea.extent.width  = width;
    begin_info.renderArea.extent.height = height;
    begin_info.clearValueCount          = (uint32_t) (std::size(clearValues));

    /// we define the clearValue struct as same as vulkan struct
    begin_info.pClearValues = (VkClearValue *) std::data(clearValues);

    VkRenderPassAttachmentBeginInfo renderPassAttachmentBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO};

    auto renderTargetImageViews = renderTargets |
                                  std::views::transform([](const AN::RenderTarget *renderTarget) {
                                      return ((VK::RenderTarget *) renderTarget->getImpl())->getImageView();
                                  });

    std::vector<VkImageView> attachments(renderTargetImageViews.begin(), renderTargetImageViews.end());

    renderPassAttachmentBeginInfo.attachmentCount = attachments.size();
    renderPassAttachmentBeginInfo.pAttachments    = attachments.data();

    begin_info.pNext = &renderPassAttachmentBeginInfo;

    vkCmdBeginRenderPass(_commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    subpassIndex = 0;
}

void CommandBuffer::endRenderPass() {
    vkCmdEndRenderPass(_commandBuffer);
}

void CommandBuffer::nextSubPass() {
    vkCmdNextSubpass(_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    subpassIndex += 1;
}

void CommandBuffer::setViewport(const Viewport &viewport) {
    VkViewport vkViewport;
    vkViewport.x = viewport.originX;
    vkViewport.y = viewport.originY;
    vkViewport.width = viewport.width;
    vkViewport.height = viewport.height;
    vkViewport.minDepth = viewport.znear;
    vkViewport.maxDepth = viewport.zfar;
    vkCmdSetViewport(_commandBuffer, 0, 1, &vkViewport);
}

void CommandBuffer::setScissor(const ScissorRect &scissorRect) {
    VkRect2D rect;
    rect.offset.x = scissorRect.x;
    rect.offset.y = scissorRect.y;
    rect.extent.width = scissorRect.width;
    rect.extent.height = scissorRect.height;
    vkCmdSetScissor(_commandBuffer, 0, 1, &rect);
}

void CommandBuffer::setCullMode(CullMode cullMode) {
    vkCmdSetCullMode(_commandBuffer, toVkRenderType(cullMode));
}

void CommandBuffer::setRenderPipelineState(AN::RenderPipelineState &renderPipelineState) {
//    if (currentPipeline != (AN::VK::RenderPipeline *)&renderPipelineState) {
//        currentPipeline = (AN::VK::RenderPipeline *)&renderPipelineState;
//        vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline->vkPipeline());
//    }
}

void CommandBuffer::bindUniformBuffer(uint32_t binding, uint64_t offset, uint64_t size, VkBuffer uniformBuffer, uint32_t set, uint32_t arrayElement) {
    resourceBindingState.bind_buffer(uniformBuffer, offset, size, set, binding, arrayElement);
}
void CommandBuffer::bindImageView(uint32_t binding, VkImageView imageView, bool isDepthStencil,
                                  uint32_t set, uint32_t arrayElement) {
    resourceBindingState.bind_image(imageView, set, binding, isDepthStencil, arrayElement);
}
void CommandBuffer::bindSampler(uint32_t binding, VkSampler sampler, uint32_t set, uint32_t arrayElement) {
    resourceBindingState.bind_sampler(sampler, set, binding, arrayElement);
}

void CommandBuffer::bindIndexBuffer(VkIndexType type, uint64_t offset, VkBuffer indexBuffer) {
    vkCmdBindIndexBuffer(_commandBuffer, indexBuffer, offset, type);
}

void CommandBuffer::bindVertexBuffer(uint32_t binding, uint64_t offset, VkBuffer vertexBuffer) {
    vkCmdBindVertexBuffers(_commandBuffer, binding, 1, &vertexBuffer, &offset);
}

void CommandBuffer::bindVertexBuffer(uint32_t binding, uint32_t bindingCount, const uint64_t *offset, VkBuffer *vertexBuffer) {
    vkCmdBindVertexBuffers(_commandBuffer, binding, bindingCount, vertexBuffer, offset);
}

void CommandBuffer::pushConstants(uint32_t offset, uint32_t size, const void *data) {
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

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) {
    flushDescriptorState();
    vkCmdDrawIndexed(_commandBuffer, indexCount, 0, indexOffset, (int32_t)vertexOffset, 0);
}

void CommandBuffer::draw(uint32_t count) {
    flushDescriptorState();
    vkCmdDraw(_commandBuffer, count, 1, 0, 0);
}

void CommandBuffer::flushDescriptorState() {
    const auto &pipeline_layout = currentPipeline->getPipelineLayout();

    auto descriptorSetsToUpdate = pipeline_layout.getDescriptorSetLayouts() |
                                  std::views::keys |
                                  std::views::filter([this, &pipeline_layout](auto set) {
                                      auto it = descriptor_set_layout_binding_state.find(set);
                                      if (it != descriptor_set_layout_binding_state.end()) {
                                          return it->second->vkDescriptorSetLayout() != pipeline_layout.getDescriptorSetLayout(set).vkDescriptorSetLayout();
                                      }
                                      return false;
                                  });

    update_descriptor_sets.assign(descriptorSetsToUpdate.begin(), descriptorSetsToUpdate.end());

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
                (std::ranges::find(update_descriptor_sets, descriptor_set_id) == std::ranges::end(update_descriptor_sets))) {
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

                            buffer_info.buffer = resource_info.buffer;
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
                            image_info.sampler   = sampler;
                            image_info.imageView = image_view;

                            if (image_view != nullptr) {
                                // Add image layout info based on descriptor type
                                switch (binding_info->descriptorType) {
                                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                                    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                                        if (resource_info.isDepthStencil) {
                                            image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                                        } else {
                                            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                        }
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




            VkDescriptorSet descriptorSet = GetDescriptorSetManager().descriptorSet(descriptorSetInfo);

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

void CommandBuffer::copyBufferToBuffer(VkBuffer srcBuffer, uint64_t srcOffset, VkBuffer dstBuffer, uint64_t dstOffset, uint64_t size) {
    VkBufferCopy copyRegion;
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;
    vkCmdCopyBuffer(_commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void CommandBuffer::blitImage(VkImage srcImage, Int32 srcWidth, Int32 srcHeight, UInt32 srcMipLevel,
                              VkImage dstImage, Int32 dstWidth, Int32 dstHeight, UInt32 dstMipLevel,
                              VkImageAspectFlags aspectMask,
                              VkFilter filter,
                              UInt32 layerCount) {
    VkImageBlit blit{};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = { srcWidth, srcHeight, 1};
    blit.srcSubresource.aspectMask = aspectMask;
    blit.srcSubresource.mipLevel = srcMipLevel;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = layerCount;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = { dstWidth, dstHeight, 1 };
    blit.dstSubresource.aspectMask = aspectMask;
    blit.dstSubresource.mipLevel = dstMipLevel;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = layerCount;

    vkCmdBlitImage(_commandBuffer,
                   srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit,
                   filter);
}

void CommandBuffer::blitTexture(TextureID srcTexID, Int32 srcWidth, Int32 srcHeight, UInt32 srcMipLevel,
                              TextureID dstTexID, Int32 dstWidth, Int32 dstHeight, UInt32 dstMipLevel,
                              TextureAspectFlags aspectMask, BlitFilter filter) {
    Texture *srcTex = GetTextureManager().getTexture(srcTexID);
    Texture *dstTex = GetTextureManager().getTexture(dstTexID);

    if (!srcTex || !dstTex) return;

    blitImage(srcTex->image, srcWidth, srcHeight, srcMipLevel,
              dstTex->image, dstWidth, dstHeight, dstMipLevel,
              toVkRenderType((TextureAspectFlagBits)aspectMask),
              toVkRenderType(filter));

}

void CommandBuffer::resolveTexture() {

}


void CommandBuffer::textureBarrier(const TextureBarrier &textureBarrier) {
    Texture *tex = GetTextureManager().getTexture(textureBarrier.textureID);

    if (tex == nullptr) return;

    VkImageMemoryBarrier imageMemoryBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    imageMemoryBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.srcAccessMask                   =
            toVkRenderType(static_cast<AccessFlagBits>(textureBarrier.srcAccessMask));
    imageMemoryBarrier.dstAccessMask                   =
            toVkRenderType(static_cast<AccessFlagBits>(textureBarrier.dstAccessMask));
    imageMemoryBarrier.oldLayout                       = toVkRenderType(textureBarrier.oldLayout);
    imageMemoryBarrier.newLayout                       = toVkRenderType(textureBarrier.newLayout);
    imageMemoryBarrier.image                           = tex->image;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount     = 1;
    imageMemoryBarrier.subresourceRange.baseMipLevel   = textureBarrier.baseMipLevel;
    imageMemoryBarrier.subresourceRange.levelCount     = textureBarrier.levelCount;
    imageMemoryBarrier.subresourceRange.aspectMask     =
            toVkRenderType(static_cast<TextureAspectFlagBits>(textureBarrier.aspectFlag));

    vkCmdPipelineBarrier(_commandBuffer,
                         toVkRenderType(static_cast<PipelineStageFlagBits>(textureBarrier.srcStageFlag)),
                         toVkRenderType(static_cast<PipelineStageFlagBits>(textureBarrier.dstStageFlag)),
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &imageMemoryBarrier);
}

void CommandBuffer::textureBarrier(std::span<const TextureBarrier> textureBarriers) {
    std::vector<VkImageMemoryBarrier2> imageMemoryBarriers;
    imageMemoryBarriers.reserve(textureBarriers.size());

    for (const TextureBarrier &textureBarrier : textureBarriers) {
        Texture *tex = GetTextureManager().getTexture(textureBarrier.textureID);

        if (tex == nullptr) continue;

        VkImageMemoryBarrier2 imageMemoryBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imageMemoryBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.srcAccessMask                   =
                toVkRenderType(static_cast<AccessFlagBits>(textureBarrier.srcAccessMask));
        imageMemoryBarrier.dstAccessMask                   =
                toVkRenderType(static_cast<AccessFlagBits>(textureBarrier.dstAccessMask));
        imageMemoryBarrier.oldLayout                       = toVkRenderType(textureBarrier.oldLayout);
        imageMemoryBarrier.newLayout                       = toVkRenderType(textureBarrier.newLayout);
        imageMemoryBarrier.image                           = tex->image;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount     = 1;
        imageMemoryBarrier.subresourceRange.baseMipLevel   = textureBarrier.baseMipLevel;
        imageMemoryBarrier.subresourceRange.levelCount     = textureBarrier.levelCount;
        imageMemoryBarrier.subresourceRange.aspectMask     =
                toVkRenderType(static_cast<TextureAspectFlagBits>(textureBarrier.aspectFlag));

        imageMemoryBarrier.srcStageMask = toVkRenderType(static_cast<PipelineStageFlagBits>(textureBarrier.dstStageFlag));
        imageMemoryBarrier.dstStageMask = toVkRenderType(static_cast<PipelineStageFlagBits>(textureBarrier.dstStageFlag));
        imageMemoryBarriers.push_back(imageMemoryBarrier);
    }

    VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependencyInfo.imageMemoryBarrierCount = imageMemoryBarriers.size();
    dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();

    vkCmdPipelineBarrier2(_commandBuffer, &dependencyInfo);
}

}

