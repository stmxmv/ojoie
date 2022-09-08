//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/UniformBuffer.hpp"
#include "Render/Renderer.hpp"
#include "Render/private/vulkan.hpp"
#include "Render/RenderPipeline.hpp"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace AN::RC {


struct UniformBuffer::Impl {

    VmaAllocator allocator;

    VkBuffer uniformBuffer;
    VmaAllocation uniformBufferAllocation;
    uint64_t bufferSize;
    uint64_t padSize;

    uint32_t frameCount;
    uint32_t maxFrameInFlight;

    void *mappedBuffer;
    bool writeOnly;
    bool isCoherent;
};


UniformBuffer::UniformBuffer() : impl(new Impl{}) {}

UniformBuffer::~UniformBuffer() {
    deinit();
    delete impl;
}

UniformBuffer &UniformBuffer::operator=(UniformBuffer &&other) noexcept {
    if (this == &other) {
        return *this;
    }
    delete impl;
    _size      = other._size;
    impl       = other.impl;
    other.impl = nullptr;
    return *this;
}

static size_t pad_uniform_buffer_size(size_t originalSize, size_t minUboAlignment) {
    // Calculate required alignment based on minimum device offset alignment
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0) {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    return alignedSize;
}

bool UniformBuffer::init(uint64_t size, bool writeOnly) {
    const RenderContext &context = GetRenderer().getRenderContext();
    _size = size;
    impl->allocator = context.graphicContext->vmaAllocator;
    impl->maxFrameInFlight = context.maxFrameInFlight;
    impl->padSize = pad_uniform_buffer_size(size, context.graphicContext->gpuProperties->limits.minUniformBufferOffsetAlignment);
    impl->bufferSize = impl->maxFrameInFlight * impl->padSize;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = impl->bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (writeOnly) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    impl->writeOnly = writeOnly;

    VmaAllocationInfo resultInfo;

    if (VK_SUCCESS != vmaCreateBuffer(context.graphicContext->vmaAllocator,
                                      &bufferInfo,
                                      &allocInfo,
                                      &impl->uniformBuffer,
                                      &impl->uniformBufferAllocation, &resultInfo)) {
        ANLog("Fail to create uniform buffer");
        return false;
    }

    impl->mappedBuffer = resultInfo.pMappedData;

    VkMemoryPropertyFlags memoryPropertyFlags;
    vmaGetMemoryTypeProperties(impl->allocator, resultInfo.memoryType, &memoryPropertyFlags);

    impl->isCoherent = memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return true;
}

void UniformBuffer::deinit() {
    if (impl && impl->uniformBuffer) {
        vmaDestroyBuffer(impl->allocator, impl->uniformBuffer, impl->uniformBufferAllocation);
        impl->uniformBuffer = nullptr;
    }
}

UniformBuffer UniformBuffer::copy() const {
    UniformBuffer clone;
    clone.init(_size);

    memcpy(impl->mappedBuffer, clone.impl->mappedBuffer, impl->bufferSize);

    return clone;
}

void *UniformBuffer::content() {
    const RenderContext &context = GetRenderer().getRenderContext();
    impl->frameCount = context.frameCount;

    if (!impl->isCoherent && !impl->writeOnly) {

        vmaInvalidateAllocation(impl->allocator,
                                impl->uniformBufferAllocation,
                                impl->padSize * (impl->frameCount % impl->maxFrameInFlight),
                                impl->padSize);
    }

    return (char *)impl->mappedBuffer + impl->padSize * (impl->frameCount % impl->maxFrameInFlight);
}

void *UniformBuffer::getUnderlyingBuffer() {
    return (void *)impl->uniformBuffer;
}

uint32_t UniformBuffer::getOffset() {
    if (!impl->isCoherent) {
        vmaFlushAllocation(impl->allocator,
                           impl->uniformBufferAllocation,
                           impl->padSize * (impl->frameCount % impl->maxFrameInFlight),
                           impl->padSize);
    }
    return (impl->frameCount % impl->maxFrameInFlight) * impl->padSize;
}


}