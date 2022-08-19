//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/IndexBuffer.hpp"
#include "Render/Renderer.hpp"
#include "Render/private/vulkan.hpp"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>


namespace AN::RC {

struct IndexBuffer::Impl {
    VmaAllocator allocator;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferAllocation;
};

IndexBuffer::IndexBuffer() : impl(new Impl{}) {}

IndexBuffer::~IndexBuffer() {
    delete impl;
}

bool IndexBuffer::initStatic(void *indices, uint64_t bytes) {
    const RenderContext &context = GetRenderer().getRenderContext();

    impl->allocator = context.graphicContext->vmaAllocator;

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

    if (VK_SUCCESS != vmaCreateBuffer(context.graphicContext->vmaAllocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingBufferMemory, nullptr)) {
        ANLog("fail to create staging buffer for vertex buffer");
        return false;
    }

    void *data;
    vmaMapMemory(context.graphicContext->vmaAllocator, stagingBufferMemory, &data);
    memcpy(data, indices, bytes);
    vmaUnmapMemory(context.graphicContext->vmaAllocator, stagingBufferMemory);

    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    allocInfo.flags = 0;


    if (VK_SUCCESS != vmaCreateBuffer(context.graphicContext->vmaAllocator, &bufferInfo, &allocInfo, &impl->indexBuffer, &impl->indexBufferAllocation, nullptr)) {
        ANLog("fail to create vertex buffer");
        return false;
    }


    CopyBuffer(context, stagingBuffer, impl->indexBuffer, bytes);

    vmaDestroyBuffer(context.graphicContext->vmaAllocator, stagingBuffer, stagingBufferMemory);

    return true;
}

void IndexBuffer::deinit() {
    vmaDestroyBuffer(impl->allocator, impl->indexBuffer, impl->indexBufferAllocation);
}

void IndexBuffer::bind(IndexType type, uint64_t offset) {
    VkIndexType vkIndexType;
    size_t elementSize;
    if (type == IndexType::UInt32) {
        vkIndexType = VK_INDEX_TYPE_UINT32;
        elementSize = sizeof(uint32_t);
    } else {
        // UInt16
        vkIndexType = VK_INDEX_TYPE_UINT16;
        elementSize = sizeof(uint16_t);
    }
    vkCmdBindIndexBuffer(GetRenderer().getRenderContext().graphicContext->commandBuffer, impl->indexBuffer, offset * elementSize, vkIndexType);
}

}