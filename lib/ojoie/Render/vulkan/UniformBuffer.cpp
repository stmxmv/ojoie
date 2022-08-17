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

    uint32_t uniformBufferIndex;
    uint32_t frameCount;
    uint32_t maxFrameInFlight;
};


UniformBuffer::UniformBuffer() : impl(new Impl{}) {}

UniformBuffer::~UniformBuffer() {
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

bool UniformBuffer::init(uint64_t size) {
    const RenderContext &context = GetRenderer().getRenderContext();
    _size = size;
    impl->allocator = context.graphicContext->vmaAllocator;
    impl->maxFrameInFlight = context.maxFrameInFlight;
    impl->padSize = pad_uniform_buffer_size(size, context.graphicContext->gpuProperties.limits.minUniformBufferOffsetAlignment);
    impl->bufferSize = impl->maxFrameInFlight * impl->padSize;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = impl->bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (VK_SUCCESS != vmaCreateBuffer(context.graphicContext->vmaAllocator,
                                      &bufferInfo,
                                      &allocInfo,
                                      &impl->uniformBuffer,
                                      &impl->uniformBufferAllocation, nullptr)) {
        ANLog("Fail to create uniform buffer");
        return false;
    }

    return true;
}

void UniformBuffer::deinit() {
    vmaDestroyBuffer(impl->allocator, impl->uniformBuffer, impl->uniformBufferAllocation);
}

UniformBuffer UniformBuffer::copy() const {
    UniformBuffer clone;
    clone.init(_size);

    void *data;
    vmaMapMemory(impl->allocator, impl->uniformBufferAllocation, &data);

    void *dataClone;
    vmaMapMemory(impl->allocator, clone.impl->uniformBufferAllocation, &dataClone);

    memcpy(dataClone, data, impl->bufferSize);

    vmaUnmapMemory(impl->allocator, impl->uniformBufferAllocation);
    vmaUnmapMemory(impl->allocator, clone.impl->uniformBufferAllocation);

    return clone;
}

void *UniformBuffer::mapMemory() {
    void *data;
    vmaMapMemory(impl->allocator, impl->uniformBufferAllocation, &data);
    data = (char *)data + impl->padSize * impl->uniformBufferIndex;
    return data;
}

void UniformBuffer::unMapMemory() {
    vmaUnmapMemory(impl->allocator, impl->uniformBufferAllocation);
}

void *UniformBuffer::getUnderlyingBuffer() {
    return (void *)impl->uniformBuffer;
}

uint32_t UniformBuffer::getOffset() {
    const RenderContext &context = GetRenderer().getRenderContext();
    if (context.frameCount != impl->frameCount) {
        impl->frameCount = context.frameCount;
        impl->uniformBufferIndex = (impl->uniformBufferIndex + 1) % impl->maxFrameInFlight;
    }
    return impl->uniformBufferIndex * impl->padSize;
}


}