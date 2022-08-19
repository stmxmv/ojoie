//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/VertexBuffer.hpp"
#include "Render/Renderer.hpp"
#include "Render/private/vulkan.hpp"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace AN::RC {

struct VertexBuffer::Impl {
    VmaAllocator allocator;
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;

    /// dynamic
    uint32_t maxFrameInFlight;
    uint32_t frameCount;
    uint64_t padSize;
    void *mappedBuffer;
    bool writeOnly;
    bool isCoherent;
};

VertexBuffer::VertexBuffer() : impl(new Impl{}) {}

VertexBuffer::~VertexBuffer() {
    delete impl;
}

VertexBuffer &VertexBuffer::operator=(VertexBuffer &&other) noexcept {
    if (this == &other) {
        return *this;
    }
    delete impl;
    impl = other.impl;
    other.impl = nullptr;
    return *this;
}

bool VertexBuffer::initCommon() {
    const RenderContext &context = GetRenderer().getRenderContext();
    impl->allocator = context.graphicContext->vmaAllocator;
    return true;
}

bool VertexBuffer::initStatic(void *vertices, uint64_t bytes) {
    if (!initCommon()) {
        return false;
    }

    const RenderContext &context = GetRenderer().getRenderContext();

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bytes;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if (VK_SUCCESS != vmaCreateBuffer(impl->allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingBufferMemory, nullptr)) {
        ANLog("fail to create staging buffer for vertex buffer");
        return false;
    }

    void *data;
    vmaMapMemory(impl->allocator, stagingBufferMemory, &data);
    memcpy(data, vertices, bytes);
    vmaUnmapMemory(impl->allocator, stagingBufferMemory);

    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.flags = 0;

    if (VK_SUCCESS != vmaCreateBuffer(impl->allocator, &bufferInfo, &allocInfo, &impl->vertexBuffer, &impl->vertexBufferAllocation, nullptr)) {
        ANLog("fail to create vertex buffer");
        return false;
    }


    CopyBuffer(context, stagingBuffer, impl->vertexBuffer, bytes);

    vmaDestroyBuffer(context.graphicContext->vmaAllocator, stagingBuffer, stagingBufferMemory);

    return true;
}


bool VertexBuffer::initDynamic(uint64_t bytes, bool writeOnly) {
    if (!initCommon()) {
        return false;
    }

    const RenderContext &context = GetRenderer().getRenderContext();
    impl->maxFrameInFlight = context.maxFrameInFlight;
    impl->padSize = (bytes + 256 - 1) & ~(256 - 1);

    bytes = impl->padSize * impl->maxFrameInFlight;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bytes;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
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

    if (VK_SUCCESS != vmaCreateBuffer(impl->allocator, &bufferInfo, &allocInfo, &impl->vertexBuffer, &impl->vertexBufferAllocation, &resultInfo)) {
        ANLog("fail to create vertex buffer");
        return false;
    }

    impl->mappedBuffer = resultInfo.pMappedData;

    VkMemoryPropertyFlags memoryPropertyFlags;
    vmaGetMemoryTypeProperties(impl->allocator, resultInfo.memoryType, &memoryPropertyFlags);

    impl->isCoherent = memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return true;
}

void *VertexBuffer::content() {
    if (impl->maxFrameInFlight) {
        /// dynamic vertex buffer
        const RenderContext &context = GetRenderer().getRenderContext();
        impl->frameCount = context.frameCount;

        if (!impl->isCoherent && !impl->writeOnly) {

            vmaInvalidateAllocation(impl->allocator,
                                    impl->vertexBufferAllocation,
                                    impl->padSize * (impl->frameCount % impl->maxFrameInFlight),
                                    impl->padSize);
        }
    }
    return (char *)impl->mappedBuffer + (impl->frameCount % impl->maxFrameInFlight) * impl->padSize;
}


void VertexBuffer::deinit() {
    vmaDestroyBuffer(impl->allocator, impl->vertexBuffer, impl->vertexBufferAllocation);
}

void VertexBuffer::bind(uint64_t offset) {
    const RenderContext &context = GetRenderer().getRenderContext();
    if (impl->maxFrameInFlight) {
        /// dynamic vertex buffer
        if (!impl->isCoherent) {
            vmaFlushAllocation(impl->allocator,
                               impl->vertexBufferAllocation,
                               impl->padSize * (impl->frameCount % impl->maxFrameInFlight),
                               impl->padSize);
        }

        offset = offset + impl->padSize * (impl->frameCount % impl->maxFrameInFlight);
    }
    vkCmdBindVertexBuffers(context.graphicContext->commandBuffer, 0, 1, &impl->vertexBuffer, &offset);
}

}