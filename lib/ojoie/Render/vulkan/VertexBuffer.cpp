//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/VertexBuffer.hpp"
#include "Render/Renderer.hpp"
#include "Render/private/vulkan.hpp"

#include "Render/Buffer.hpp"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace AN::RC {



bool VertexBuffer::init(uint64_t bytes) {

    const RenderContext &context = GetRenderer().getRenderContext();

    RC::BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = bytes;
    bufferDescriptor.memoryUsage = MemoryUsage::AutoPreferDevice;
    bufferDescriptor.bufferUsage = BufferUsageFlag::TransferDestination | BufferUsageFlag::VertexBuffer;
    
    if (!vertexBuffer.init(context.device, bufferDescriptor)) {
        return false;
    }

    return true;
}


bool VertexBuffer::initDynamic(uint64_t bytes, bool writeOnly) {
  
    const RenderContext &context = GetRenderer().getRenderContext();
    maxFrameInFlight = context.maxFrameInFlight;
    padSize = (bytes + 256 - 1) & ~(256 - 1);
    _writeOnly = writeOnly;
    
    bytes = padSize * maxFrameInFlight;
    
    RC::BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = bytes;
    bufferDescriptor.memoryUsage = MemoryUsage::Auto;
    bufferDescriptor.bufferUsage = BufferUsageFlag::VertexBuffer;
    if (writeOnly) {
        bufferDescriptor.allocationFlag = AllocationFlag::HostAccessSequentialWrite;
    } else {
        bufferDescriptor.allocationFlag = AllocationFlag::HostAccessRandom;
    } 
    
    if (!vertexBuffer.init(context.device, bufferDescriptor)) {
        return false;
    }

    mappedBuffer = vertexBuffer.map();
    return true;
}

void *VertexBuffer::content() {
    if (maxFrameInFlight) {
        /// dynamic vertex buffer
        const RenderContext &context = GetRenderer().getRenderContext();
        frameCount = context.frameCount;

        if (!_writeOnly) {
            vertexBuffer.invalidate();
        }
    }
    return (char *)mappedBuffer + (frameCount % maxFrameInFlight) * padSize;
}


void VertexBuffer::deinit() {
    vertexBuffer.deinit();
}

uint64_t VertexBuffer::getBufferOffset(uint64_t offset) {
    if (maxFrameInFlight) {
        /// dynamic vertex buffer
        
        vertexBuffer.flush();

        offset = offset + padSize * (frameCount % maxFrameInFlight);
    }
    return offset;
}

}