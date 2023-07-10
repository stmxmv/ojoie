//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/UniformBuffer.hpp"
#include "Render/Renderer.hpp"
#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/RenderPipelineState.hpp"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace AN::RC {


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
    maxFrameInFlight = context.maxFrameInFlight;
    padSize = pad_uniform_buffer_size(size, context.graphicContext->device->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment);
    bufferSize = maxFrameInFlight * padSize;

    RC::BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = bufferSize;
    bufferDescriptor.bufferUsage = BufferUsageFlag::UniformBuffer;
    bufferDescriptor.memoryUsage = MemoryUsage::Auto;
    if (writeOnly) {
        bufferDescriptor.allocationFlag = AllocationFlag::HostAccessSequentialWrite;
    } else {
        bufferDescriptor.allocationFlag = AllocationFlag::HostAccessRandom;
    }

    if (!uniformBuffer.init(context.device, bufferDescriptor)) {
        return false;
    }

    _writeOnly = writeOnly;

    mappedBuffer = uniformBuffer.map();

    return true;
}

void UniformBuffer::deinit() {
    uniformBuffer.deinit();
}

UniformBuffer UniformBuffer::copy() const {
    UniformBuffer clone;
    clone.init(_size);

    memcpy(mappedBuffer, clone.mappedBuffer, bufferSize);

    return clone;
}

void *UniformBuffer::content() {
    const RenderContext &context = GetRenderer().getRenderContext();
    frameCount = context.frameCount;

    if (!_writeOnly) {
        uniformBuffer.invalidate();
    }

    return (char *)mappedBuffer + padSize * (frameCount % maxFrameInFlight);
}

uint32_t UniformBuffer::getOffset() {
    uniformBuffer.flush();
    return (frameCount % maxFrameInFlight) * padSize;
}


}