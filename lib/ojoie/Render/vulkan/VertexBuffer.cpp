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
};

VertexBuffer::VertexBuffer() : impl(new Impl{}) {}

VertexBuffer::~VertexBuffer() {
    delete impl;
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
    allocInfo.flags = 0;


    if (VK_SUCCESS != vmaCreateBuffer(impl->allocator, &bufferInfo, &allocInfo, &impl->vertexBuffer, &impl->vertexBufferAllocation, nullptr)) {
        ANLog("fail to create vertex buffer");
        return false;
    }


    CopyBuffer(context, stagingBuffer, impl->vertexBuffer, bytes);

    vmaDestroyBuffer(context.graphicContext->vmaAllocator, stagingBuffer, stagingBufferMemory);

    return true;
}


bool VertexBuffer::initDynamic(uint64_t bytes) {
    if (!initCommon()) {
        return false;
    }

    const RenderContext &context = GetRenderer().getRenderContext();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bytes;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (VK_SUCCESS != vmaCreateBuffer(impl->allocator, &bufferInfo, &allocInfo, &impl->vertexBuffer, &impl->vertexBufferAllocation, nullptr)) {
        ANLog("fail to create vertex buffer");
        return false;
    }

    return true;
}

void *VertexBuffer::mapMemory() {
    void *data;
    vmaMapMemory(impl->allocator, impl->vertexBufferAllocation, &data);
    return data;
}

void VertexBuffer::unMapMemory() {
    vmaUnmapMemory(impl->allocator, impl->vertexBufferAllocation);
}

void VertexBuffer::deinit() {
    vmaDestroyBuffer(impl->allocator, impl->vertexBuffer, impl->vertexBufferAllocation);
}

void VertexBuffer::bind(uint64_t offset) {
    VkDeviceSize offsets = offset;
    vkCmdBindVertexBuffers(GetRenderer().getRenderContext().graphicContext->commandBuffer, 0, 1, &impl->vertexBuffer, &offsets);
}

}