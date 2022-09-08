//
// Created by Aleudillonam on 9/6/2022.
//

#ifndef OJOIE_RENDERCOMMANDENCODER_HPP
#define OJOIE_RENDERCOMMANDENCODER_HPP

namespace AN::VK {

struct ImageMemoryBarrier {
    VkPipelineStageFlags srcStageMask;

    VkPipelineStageFlags dstStageMask;

    VkAccessFlags srcAccessMask;

    VkAccessFlags dstAccessMask;

    VkImageLayout oldLayout;

    VkImageLayout newLayout;
};

class RenderPassCommandEncoder {

    RenderPass *renderPass;

    FrameBuffer *frameBuffer;

    Layer *_layer;

    VkCommandBuffer _commandBuffer;
public:

    template<typename ClearValues>
    RenderPassCommandEncoder(Layer &layer, ClearValues &&clearValues, VkCommandBuffer commandBuffer, RenderPassDescriptor &renderPassDescriptor) {
        _layer = &layer;
        _commandBuffer = commandBuffer;
        renderPass = &layer.getDevice().getRenderResourceCache().newRenderPass(renderPassDescriptor);

        frameBuffer = &layer.getDevice().getRenderResourceCache().newFrameBuffer(layer.getActiveFrame().getRenderTarget(), *renderPass);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        const auto &views = layer.getActiveFrame().getRenderTarget().views;
        // Image 0 is the swapchain
        {

            ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            imageBarrier(views[0], memory_barrier);

            // Skip 1 as it is handled later as a depth-stencil attachment
            for (size_t i = 2; i < views.size(); ++i) {
                imageBarrier(views[i], memory_barrier);
            }
        }

        {
            ImageMemoryBarrier memory_barrier{};
            memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.newLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            memory_barrier.srcAccessMask = 0;
            memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

            imageBarrier(views[1], memory_barrier);
        }

        // Begin render pass
        VkRenderPassBeginInfo begin_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        begin_info.renderPass        = renderPass->vkRenderPass();
        begin_info.framebuffer       = frameBuffer->vkFramebuffer();
        begin_info.renderArea.extent = layer.getActiveFrame().getRenderTarget().extent;
        begin_info.clearValueCount   = (uint32_t)(std::size(clearValues));
        begin_info.pClearValues      = std::data(clearValues);

        vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    }

    void imageBarrier(const ImageView &imageView, const ImageMemoryBarrier &memoryBarrier) {

        VkImageMemoryBarrier image_memory_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        image_memory_barrier.oldLayout        = memoryBarrier.oldLayout;
        image_memory_barrier.newLayout        = memoryBarrier.newLayout;
        image_memory_barrier.image            = imageView.getImage()->vkImage();
        image_memory_barrier.subresourceRange = imageView.getSubresourceRange();
        image_memory_barrier.srcAccessMask    = memoryBarrier.srcAccessMask;
        image_memory_barrier.dstAccessMask    = memoryBarrier.dstAccessMask;

        VkPipelineStageFlags src_stage_mask = memoryBarrier.srcStageMask;
        VkPipelineStageFlags dst_stage_mask = memoryBarrier.dstStageMask;

        vkCmdPipelineBarrier(
                _commandBuffer,
                src_stage_mask,
                dst_stage_mask,
                0,
                0, nullptr,
                0, nullptr,
                1,
                &image_memory_barrier);
    }

    /// \brief end render encoding, therefore end renderPass
    void endRenderPass() {
        vkCmdEndRenderPass(_commandBuffer);

        const auto &views = _layer->getActiveFrame().getRenderTarget().views;
        ImageMemoryBarrier memory_barrier{};
        memory_barrier.oldLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        memory_barrier.newLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        memory_barrier.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        memory_barrier.dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        imageBarrier(views[0], memory_barrier);
    }

    /// \brief helper method to submit buffer to queue and present
    void submitAndPresent() {

        if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
            ANLog("failed to end recording command buffer!");
        }

        _layer->submit(_commandBuffer);
        _layer->present();
    }

    RenderPass &getRenderPass() const {
        return *renderPass;
    }
};

}

#endif//OJOIE_RENDERCOMMANDENCODER_HPP
