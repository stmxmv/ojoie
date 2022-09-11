//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/IndexBuffer.hpp"
#include "Render/Renderer.hpp"


namespace AN::RC {




bool IndexBuffer::init(uint64_t bytes) {
    const RenderContext &context = GetRenderer().getRenderContext();

    RC::BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = bytes;
    bufferDescriptor.bufferUsage = BufferUsageFlag::IndexBuffer | BufferUsageFlag::TransferDestination;
    bufferDescriptor.memoryUsage = MemoryUsage::Auto;

    if (!indexBuffer.init(context.device, bufferDescriptor)) {
        return false;
    }

    return true;
}

bool IndexBuffer::initDynamic(uint64_t bytes, bool writeOnly) {
    const RenderContext &context = GetRenderer().getRenderContext();
    maxFrameInFlight = context.maxFrameInFlight;
    padSize = (bytes + 256 - 1) & ~(256 - 1);
    _writeOnly = writeOnly;

    bytes = padSize * maxFrameInFlight;

    RC::BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = bytes;
    bufferDescriptor.memoryUsage = MemoryUsage::Auto;
    bufferDescriptor.bufferUsage = BufferUsageFlag::IndexBuffer;
    if (writeOnly) {
        bufferDescriptor.allocationFlag = AllocationFlag::HostAccessSequentialWrite;
    } else {
        bufferDescriptor.allocationFlag = AllocationFlag::HostAccessRandom;
    }
    if (!indexBuffer.init(context.device, bufferDescriptor)) {
        return false;
    }

    mappedBuffer = indexBuffer.map();
    return true;
}

void IndexBuffer::deinit() {
    indexBuffer.deinit();
}

void *IndexBuffer::content() {
    if (maxFrameInFlight) {
        /// dynamic index buffer
        const RenderContext &context = GetRenderer().getRenderContext();
        frameCount = context.frameCount;

        if (!_writeOnly) {
            indexBuffer.invalidate();
        }
    }
    return (char *)mappedBuffer + (frameCount % maxFrameInFlight) * padSize;
}

uint64_t IndexBuffer::getBufferOffset(uint64_t offset) {
    if (maxFrameInFlight) {
        /// dynamic vertex buffer

        indexBuffer.flush();

        offset = offset + padSize * (frameCount % maxFrameInFlight);
    }
    return offset;
}

}