//
// Created by Aleudillonam on 9/12/2022.
//

#ifndef OJOIE_VK_COMMANDBUFFER_HPP
#define OJOIE_VK_COMMANDBUFFER_HPP

#include "ojoie/Render/private/vulkan/ResourceBindingState.hpp"
#include <ojoie/Render/RenderPipelineState.hpp>
#include <ojoie/Render/private/vulkan.hpp>
#include <ojoie/Render/private/vulkan/DescriptorSetManager.hpp>
#include <ojoie/Render/private/vulkan/Device.hpp>
#include <ojoie/Render/private/vulkan/Presentable.hpp>
#include <ojoie/Render/private/vulkan/RenderPipeline.hpp>
#include <ojoie/Render/private/vulkan/Semaphore.hpp>
#include <unordered_set>

namespace AN::VK {

class Queue;
class CommandPool;
class FencePool;
class SemaphorePool;
class Semaphore;
class RenderPass;
class RenderTarget;

class CommandBuffer : public AN::CommandBuffer {
    Device  *_device;
    uint32_t _queueFamilyIndex;

    VkCommandBuffer        _commandBuffer;
    CommandBufferResetMode _resetMode;
    VkFence                _fence;

    Semaphore _semaphore;
    UInt64 lastSemaphoreValue;

    std::vector<UInt64>               waitSemaphoreValues;
    std::vector<UInt64>               signalSemaphoreValues;
    std::vector<VkSemaphore>          waitSemaphores;
    std::vector<VkSemaphore>          signalSemaphores;
    std::vector<VkPipelineStageFlags> waitDstStageFlags;

    std::vector<VkSemaphore> presentWaitSemaphores;

    std::vector<VkSwapchainKHR> swapchains;
    std::vector<uint32_t>       imageIndices;

    RenderPass          *_renderPass{};
    RenderPipeline      *currentPipeline{};

    std::vector<uint8_t> stored_push_constants;

    std::vector<uint32_t> update_descriptor_sets;
    std::vector<uint32_t>        dynamic_offsets;

    ResourceBindingState                                resourceBindingState;
    std::unordered_map<uint32_t, DescriptorSetLayout *> descriptor_set_layout_binding_state;

    DescriptorSetInfo descriptorSetInfo;

    VertexDescriptor _vertexDescriptor;

    uint32_t subpassIndex;

    void flushDescriptorState();

public:
    CommandBuffer(Device                &device,
                  uint32_t               queueFamilyIndex,
                  VkCommandBuffer        commandBuffer,
                  CommandBufferResetMode resetMode,
                  VkFence                fence);

    void beginRecord();

    VkCommandBuffer vkCommandBuffer() const { return _commandBuffer; }

    uint32_t getQueueFamilyIndex() const { return _queueFamilyIndex; }

    void setFenceInternal(VkFence fence) { _fence = fence; }

    /// use waitValue when sema is timeline
    void addWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags stageFlags, UInt64 waitValue = 0) {
        waitSemaphoreValues.push_back(waitValue);
        waitDstStageFlags.push_back(stageFlags);
        waitSemaphores.push_back(semaphore);
    }

    /// use signalValue when sema is timeline
    void addSignalSemaphore(VkSemaphore semaphore, UInt64 signalValue = 0) {
        signalSemaphoreValues.push_back(signalValue);
        signalSemaphores.push_back(semaphore);
    }

    /// \brief register the presentable to present after submit this commandBuffer as soon as possiable
    /// \note  currently you can only present one Presentable before submitted
    virtual void present(const AN::Presentable &presentable) override;

    void imageBarrier(VkPipelineStageFlags srcMask, VkPipelineStageFlags dstMask,
                      const VkImageMemoryBarrier &memoryBarrier);

    void bufferBarrier(VkPipelineStageFlags srcMask, VkPipelineStageFlags dstMask,
                       const VkBufferMemoryBarrier &bufferMemoryBarrier);

    void generateMipmaps(VkImage image, UInt32 width, UInt32 height, uint32_t mipLevels);

    void copyBufferToImage(VkBuffer buffer, uint64_t bufferOffset,
                           uint64_t bufferRowLength, uint64_t bufferImageHeight,
                           VkImage image, uint32_t mipmapLevel,
                           int32_t xOffset, int32_t yOffset,
                           uint32_t width, uint32_t height,
                           VkImageAspectFlags aspectMask,
                           uint32_t           layerCount = 1);

    void copyBufferToBuffer(VkBuffer srcBuffer, uint64_t srcOffset, VkBuffer dstBuffer, uint64_t dstOffset, uint64_t size);

    /// srcImage and dstImage can be the same with different mipLevel
    void blitImage(VkImage srcImage, Int32 srcWidth, Int32 srcHeight, UInt32 srcMipLevel,
                   VkImage dstImage, Int32 dstWidth, Int32 dstHeight, UInt32 dstMipLevel,
                   VkImageAspectFlags aspectMask,
                   VkFilter           filter,
                   UInt32             layerCount = 1);

    virtual void blitTexture(TextureID srcTexID, Int32 srcWidth, Int32 srcHeight, UInt32 srcMipLevel,
                             TextureID dstTexID, Int32 dstWidth, Int32 dstHeight, UInt32 dstMipLevel,
                             TextureAspectFlags aspectMask, BlitFilter filter) override;

    // TODO
    void resolveTexture();

    virtual void beginRenderPass(UInt32 width, UInt32 height,
                                 AN::RenderPass                     &renderPass,
                                 std::span<const AN::RenderTarget *> renderTargets,
                                 std::span<ClearValue>               clearValues) override;

    virtual void nextSubPass() override;

    virtual void endRenderPass() override;

    virtual void setViewport(const Viewport &viewport) override;

    virtual void setScissor(const ScissorRect &scissorRect) override;

    virtual void setCullMode(CullMode cullMode) override;

    virtual void setRenderPipelineState(AN::RenderPipelineState &renderPipelineState) override;

    void bindUniformBuffer(uint32_t binding, uint64_t offset, uint64_t size, VkBuffer uniformBuffer,
                           uint32_t set = 0, uint32_t arrayElement = 0);

    void bindImageView(uint32_t binding, VkImageView imageView, bool isDepthStencil = false,
                       uint32_t set = 0, uint32_t arrayElement = 0);

    void bindSampler(uint32_t binding, VkSampler sampler, uint32_t set = 0, uint32_t arrayElement = 0);

    /// \param offset element offset
    void bindIndexBuffer(VkIndexType type, uint64_t offset, VkBuffer indexBuffer);

    void bindVertexBuffer(uint32_t binding, uint64_t offset, VkBuffer vertexBuffer);

    void bindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, const uint64_t *offset, VkBuffer *vertexBuffer);

    virtual void pushConstants(uint32_t offset, uint32_t size, const void *data) override;

    virtual void drawIndexed(uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;

    virtual void draw(uint32_t count) override;

    virtual void textureBarrier(const TextureBarrier &textureBarrier) override;
    virtual void textureBarrier(std::span<const TextureBarrier> textureBarriers) override;

    /// \brief submit to the queue, cannot be called concurrently, after submitting, the only operation is waiting
    /// \attention submit multiple times is not yet supported
    void submit(const Queue &queue);

    /// can be called in any thread
    virtual void submit() override;

    /// \brief wait single command buffer to complete
    virtual void waitUntilCompleted() override;

    virtual bool isCompleted() override;

    virtual void reset() override;

    virtual void debugLabelBegin(const char *name, Vector4f color) override;

    virtual void debugLabelEnd() override;

    virtual void debugLabelInsert(const char *name, Vector4f color) override;

    virtual void bindTexture(UInt32 binding, TextureID texID, ShaderStageFlags stages, UInt32 set) override {
    }
    virtual void bindSampler(UInt32 binding, const SamplerDescriptor &sampler, ShaderStageFlags stages, UInt32 set) override {
    }
    virtual void blitTexture(AN::Texture *tex, AN::RenderTarget *target) override {
    }
    virtual void resolveTexture(AN::Texture *src, UInt32 srcMipLevel, AN::Texture *dst, UInt32 dstMipLevel, PixelFormat format) override {
    }
};


}// namespace AN::VK

#endif//OJOIE_VK_COMMANDBUFFER_HPP
