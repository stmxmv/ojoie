//
// Created by Aleudillonam on 8/14/2022.
//

#include "Render/VertexBuffer.hpp"
#include "Render/Renderer.hpp"
#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/CommandPool.hpp"
#include "Render/private/vulkan/Device.hpp"
#include "Render/private/vulkan/VertexBuffer.hpp"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace AN::VK {

VertexBuffer::VertexBuffer()
    : Super(),
      _VBStreams(),
      _VBAllocation(),
      _stagingVB(),
      _stagingVBAllocation(),
      _stagingVBSemaphore(),
      _stagingVBSemaphoreValue(),
      _indexBuffer(),
      _indexAllocation(),
      _stagingIndexBuffer(),
      _stagingIndexAllocation(),
      _stagingIndexSemaphore(),
      _stagingIndexSemaphoreValue(),
      _dynVBStreams(),
      _dynIndexBuffer(),
      _currentVersion(),
      _renderVersion(),
      _indexBufferSize(),
      _vertexCount(),
      _dynIndexBufferSize(),
      _dynVertexCount() {}


void VertexBuffer::deinit() {
    for (int i = 0; i < kMaxVertexStreams; ++i) {
        if (_stagingVBSemaphore[i].isValid()) {
            _stagingVBSemaphore[i].wait(_stagingVBSemaphoreValue[i] + 1);
            _stagingVBSemaphore[i].deinit();
        }

        if (_VBStreams[i]) {
            vmaDestroyBuffer(GetDevice().vmaAllocator(), _VBStreams[i], _VBAllocation[i]);
            _VBStreams[i]    = nullptr;
            _VBAllocation[i] = nullptr;
        }

        if (_stagingVB[i]) {
            vmaDestroyBuffer(GetDevice().vmaAllocator(), _stagingVB[i], _stagingVBAllocation[i]);
            _stagingVB[i]           = nullptr;
            _stagingVBAllocation[i] = nullptr;
        }
    }

    if (_stagingIndexSemaphore.isValid()) {
        _stagingIndexSemaphore.wait(_stagingIndexSemaphoreValue + 1);
        _stagingIndexSemaphore.deinit();
    }

    if (_indexBuffer) {
        vmaDestroyBuffer(GetDevice().vmaAllocator(), _indexBuffer, _indexAllocation);
        _indexBuffer     = nullptr;
        _indexAllocation = nullptr;
    }

    if (_stagingIndexBuffer) {
        vmaDestroyBuffer(GetDevice().vmaAllocator(), _stagingIndexBuffer, _stagingIndexAllocation);
        _stagingIndexBuffer     = nullptr;
        _stagingIndexAllocation = nullptr;
    }

    for (int i = 0; i < kMaxFrameInFlight; ++i) {
        for (int j = 0; j < kMaxVertexStreams; ++j) {
            if (_dynVBStreams[i][j].buffer) {
                vmaDestroyBuffer(GetDevice().vmaAllocator(), _dynVBStreams[i][j].buffer, _dynVBStreams[i][j].allocation);
                _dynVBStreams[i][j].buffer = nullptr;
                _dynVBStreams[i][j].allocation = nullptr;
            }
        }

        if (_dynIndexBuffer[i].buffer) {
            vmaDestroyBuffer(GetDevice().vmaAllocator(), _dynIndexBuffer[i].buffer, _dynIndexBuffer[i].allocation);
            _dynIndexBuffer[i].buffer = nullptr;
            _dynIndexBuffer[i].allocation = nullptr;
        }
    }

    for (BufferInfo &info : _oldBuffers) {
        vmaDestroyBuffer(GetDevice().vmaAllocator(), info.buffer, info.allocation);
    }
    _oldBuffers.clear();
}

void VertexBuffer::cleanupOldBuffers() {
    for (auto it = _oldBuffers.begin(); it != _oldBuffers.end(); ) {
        if (_currentVersion - it->version  >= 7) {
            vmaDestroyBuffer(GetDevice().vmaAllocator(), it->buffer, it->allocation);
            it = _oldBuffers.erase(it);
        } else {
            ++it;
        }
    }
}

void VertexBuffer::setVersionAndCleanup(UInt32 version) {
    if (_currentVersion != version) {
        _currentVersion = version;
        cleanupOldBuffers();
    }
}

void VertexBuffer::pendingDestroyVertexBufferStream(int stream, int index) {
    if (_streamModes[stream] == kStreamModeDynamic) {

        if (_dynVBStreams[index][stream].buffer) {
            _oldBuffers.push_back(_dynVBStreams[index][stream]);
            _dynVBStreams[index][stream].buffer = nullptr;
        }

    } else {
        if (_VBStreams[stream]) {
            BufferInfo info{ .buffer = _VBStreams[stream],
                             .allocation = _VBAllocation[stream],
                             .version = _currentVersion };
            _oldBuffers.push_back(info);
            _VBStreams[stream]    = nullptr;
            _VBAllocation[stream] = nullptr;
        }

        // release staging VB as well here
        if (_stagingVB[stream]) {
            BufferInfo info{ .buffer = _stagingVB[stream],
                             .allocation = _stagingVBAllocation[stream],
                             .version = _currentVersion };
            _oldBuffers.push_back(info);
            _stagingVB[stream]           = nullptr;
            _stagingVBAllocation[stream] = nullptr;
        }
    }
}

void VertexBuffer::pendingDestroyIndexBuffer(int index) {
    if (_indicesDynamic) {

        if (_dynIndexBuffer[index].buffer) {
            _oldBuffers.push_back(_dynIndexBuffer[index]);
            _dynIndexBuffer[index].buffer = nullptr;
        }

    } else {
        if (_indexBuffer) {
            BufferInfo info{ .buffer = _indexBuffer,
                             .allocation = _indexAllocation,
                             .version = _currentVersion };
            _oldBuffers.push_back(info);
            _indexBuffer     = nullptr;
            _indexAllocation = nullptr;
        }

        if (_stagingIndexBuffer) {
            BufferInfo info{ .buffer = _stagingIndexBuffer,
                             .allocation = _stagingIndexAllocation,
                             .version = _currentVersion };
            _oldBuffers.push_back(info);
            _stagingIndexBuffer     = nullptr;
            _stagingIndexAllocation = nullptr;
        }
    }
}

void VertexBuffer::setVertexStreamMode(unsigned int stream, AN::StreamMode mode) {
    if (_streamModes[stream] == mode) {
        return;
    }

    Super::setVertexStreamMode(stream, mode);

    for (int i = 0; i < kMaxFrameInFlight; ++i) {
        pendingDestroyVertexBufferStream(stream, i);
    }
}

void VertexBuffer::setIndicesDynamic(bool dynamic) {
    if (_indicesDynamic == dynamic) {
        /// no-ops
        return;
    }

    Super::setIndicesDynamic(dynamic);

    /// destroy index and staging buffer if allocated
    for (int i = 0; i < kMaxFrameInFlight; ++i) {
        pendingDestroyIndexBuffer(i);
    }
}

void VertexBuffer::createStagingBuffer(VkBuffer *outBuffer, VmaAllocation *outAllocation, int size) {
    VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferCreateInfo.size  = size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo allocationCreateInfo{};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkResult result = vmaCreateBuffer(GetDevice().vmaAllocator(),
                                      &bufferCreateInfo, &allocationCreateInfo,
                                      outBuffer, outAllocation, nullptr);

    if (result != VK_SUCCESS) {
        AN_LOG(Error, "vulkan: failed to create vertex buffer of size %d, %s", size, ResultCString(result));
        return;
    }
}

void VertexBuffer::updateVertexStream(const VertexBufferData &sourceData, unsigned int stream) {
    const StreamInfo &srcStream = sourceData.streams[stream];
    int         oldSize;
    if (_streamModes[stream] == kStreamModeDynamic) {
        oldSize   = CalculateVertexStreamSize(_streams[stream], _dynVertexCount[_currentVersion % kMaxFrameInFlight]);
    } else {
        oldSize   = CalculateVertexStreamSize(_streams[stream], _vertexCount);
    }


    const int newSize = CalculateVertexStreamSize(srcStream, sourceData.vertexCount);

    _streams[stream] = srcStream;

    if (newSize == 0) {
        return;
    }

    if (_streamModes[stream] == kStreamModeDynamic && newSize > oldSize) {
        _dynVertexCount[_currentVersion % kMaxFrameInFlight] = sourceData.vertexCount;
    } else {
        _vertexCount = sourceData.vertexCount;
    }

    /// dynamic will allocate kMaxFrameInFlight number of buffers
    const bool isDynamic  = (_streamModes[stream] == kStreamModeDynamic);
    const bool useStaging = !isDynamic;

    if ((!isDynamic && _VBStreams[stream] == nullptr) ||
        (isDynamic && _dynVBStreams[frameIndex()][stream].buffer == nullptr) ||
        newSize > oldSize) {

        pendingDestroyVertexBufferStream(stream, frameIndex());

        VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.size  = newSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VkResult result;

        if (!isDynamic) {
            bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            result = vmaCreateBuffer(GetDevice().vmaAllocator(),
                                     &bufferCreateInfo, &allocationCreateInfo,
                                     &_VBStreams[stream], &_VBAllocation[stream], nullptr);

            if (result != VK_SUCCESS) {
                AN_LOG(Error, "vulkan: failed to create vertex buffer of size %d, %s", newSize, ResultCString(result));
                return;
            }

        } else {
            /// dynamic
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

            result = vmaCreateBuffer(GetDevice().vmaAllocator(),
                                     &bufferCreateInfo, &allocationCreateInfo,
                                     &_dynVBStreams[frameIndex()][stream].buffer,
                                     &_dynVBStreams[frameIndex()][stream].allocation, nullptr);

            if (result != VK_SUCCESS) {
                AN_LOG(Error, "vulkan: failed to create vertex buffer of size %d, %s", newSize, ResultCString(result));
                return;
            }
        }

        if (useStaging) {
            createStagingBuffer(&_stagingVB[stream], &_stagingVBAllocation[stream], newSize);
        }
    }

    // Don't update contents if there is no source data.
    // This is used to update the vertex declaration only, leaving buffer intact.
    // Also to create an empty buffer that is written to later.
    if (!sourceData.buffer)
        return;

    VmaAllocation mapVB = nullptr;
    void         *mappedData;
    if (useStaging) {
        mapVB = _stagingVBAllocation[stream];

        /// wait last submit complete
        if (_stagingVBSemaphore[stream].isValid()) {
            _stagingVBSemaphore[stream].wait(_stagingVBSemaphoreValue[stream] + 1);
            ++_stagingVBSemaphoreValue[stream];
        }

    } else {
        /// dynamic
        _dynVBStreams[frameIndex()][stream].version = _currentVersion;
        mapVB = _dynVBStreams[frameIndex()][stream].allocation;
    }

    VkResult result = vmaMapMemory(GetDevice().vmaAllocator(), mapVB, &mappedData);

    if (result != VK_SUCCESS) {
        AN_LOG(Error, "Cannot map vulkan vertex buffer memory %s", ResultCString(result));
        return;
    }

    CopyVertexStream(sourceData, mappedData, stream);

    vmaUnmapMemory(GetDevice().vmaAllocator(), mapVB);

    if (useStaging) {
        VK::CommandBuffer *commandBuffer = VK::GetCommandPool()
                                                   .newCommandBuffer(GetDevice().getGraphicsQueue().getFamilyIndex());

        commandBuffer->copyBufferToBuffer(_stagingVB[stream], 0, _VBStreams[stream], 0, newSize);

        VkBufferMemoryBarrier bufferMemoryBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        bufferMemoryBarrier.buffer              = _VBStreams[stream];
        bufferMemoryBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufferMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        bufferMemoryBarrier.offset              = 0;
        bufferMemoryBarrier.size                = newSize;
        bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        commandBuffer->bufferBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                     bufferMemoryBarrier);

        if (!_stagingVBSemaphore[stream].isValid()) {
            _stagingVBSemaphore[stream].init(false);
        }
        commandBuffer->addSignalSemaphore(_stagingVBSemaphore[stream].vkSemaphore(), _stagingVBSemaphoreValue[stream] + 1);
        commandBuffer->submit();
    }
}

void VertexBuffer::updateVertexData(const VertexBufferData &buffer) {
//    setVersionAndCleanup(version);
    for (unsigned stream = 0; stream < kMaxVertexStreams; stream++) {
        updateVertexStream(buffer, stream);
    }


}

void VertexBuffer::updateIndexBufferData(const IndexBufferData &sourceData) {
    if (!sourceData.indices) {
        if (_indicesDynamic) {
            _indexBufferSize = 0;
        } else {
            _dynIndexBufferSize[_currentVersion % kMaxFrameInFlight] = 0;
        }
        return;
    }

    int size = sourceData.count * kVBOIndexSize;

    VmaAllocation mapAllocation;

    if (_indicesDynamic) {
        _dynIndexBuffer[frameIndex()].version = _currentVersion;
        mapAllocation = _dynIndexBuffer[frameIndex()].allocation;
    } else {
        if (!_stagingIndexBuffer) {
            createStagingBuffer(&_stagingIndexBuffer, &_stagingIndexAllocation, _indexBufferSize);
            if (!_stagingIndexBuffer) {
                /// fail to create buffer
                return;
            }
        } else {
            /// wait last submit complete
            if (_stagingIndexSemaphore.isValid()) {
                _stagingIndexSemaphore.wait(_stagingIndexSemaphoreValue + 1);
                ++_stagingIndexSemaphoreValue;
            }
        }
        mapAllocation = _stagingIndexAllocation;
    }

    void    *mappedData;
    VkResult result = vmaMapMemory(GetDevice().vmaAllocator(), mapAllocation, &mappedData);
    if (result != VK_SUCCESS) {
        AN_LOG(Error, "Cannot map vulkan index buffer memory %s", ResultCString(result));
        return;
    }

    memcpy(mappedData, sourceData.indices, size);

    vmaUnmapMemory(GetDevice().vmaAllocator(), mapAllocation);

    if (!_indicesDynamic) {
        // upload staging buffer
        VK::CommandBuffer *commandBuffer = VK::GetCommandPool()
                                                   .newCommandBuffer(GetDevice().getGraphicsQueue().getFamilyIndex());

        commandBuffer->copyBufferToBuffer(_stagingIndexBuffer, 0, _indexBuffer, 0, _indexBufferSize);

        VkBufferMemoryBarrier bufferMemoryBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        bufferMemoryBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufferMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        bufferMemoryBarrier.offset              = 0;
        bufferMemoryBarrier.size                = _indexBufferSize;
        bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferMemoryBarrier.buffer              = _indexBuffer;

        commandBuffer->bufferBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                     bufferMemoryBarrier);

        if (!_stagingIndexSemaphore.isValid()) {
            _stagingIndexSemaphore.init(false);
        }

        commandBuffer->addSignalSemaphore(_stagingIndexSemaphore.vkSemaphore(), _stagingIndexSemaphoreValue + 1);
        commandBuffer->submit();
    }
}

void VertexBuffer::updateIndexData(const IndexBufferData &buffer) {
//    setVersionAndCleanup(version);

    int newSize = CalculateIndexBufferSize(buffer);

    if (_indicesDynamic) {
        if (newSize > _dynIndexBufferSize[_currentVersion % kMaxFrameInFlight]) {
            pendingDestroyIndexBuffer(frameIndex());
            _dynIndexBufferSize[_currentVersion % kMaxFrameInFlight] = newSize;
        }

    } else {
        if (newSize > _indexBufferSize) {
            pendingDestroyIndexBuffer(frameIndex());
            _indexBufferSize = newSize;
        }
    }


    // Create buffer if we need to
    if ((!_indicesDynamic && !_indexBuffer) ||
        (_indicesDynamic && !_dynIndexBuffer[frameIndex()].buffer)) {

        VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.size  = newSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VkResult result;

        if (!_indicesDynamic) {
            bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            result = vmaCreateBuffer(GetDevice().vmaAllocator(),
                                     &bufferCreateInfo, &allocationCreateInfo,
                                     &_indexBuffer, &_indexAllocation, nullptr);

            if (result != VK_SUCCESS) {
                AN_LOG(Error, "vulkan: failed to create vertex buffer of size %d, %s", newSize, ResultCString(result));
                return;
            }

        } else {
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

            result = vmaCreateBuffer(GetDevice().vmaAllocator(),
                                     &bufferCreateInfo, &allocationCreateInfo,
                                     &_dynIndexBuffer[frameIndex()].buffer,
                                     &_dynIndexBuffer[frameIndex()].allocation, nullptr);

            if (result != VK_SUCCESS) {
                AN_LOG(Error, "vulkan: failed to create vertex buffer of size %d, %s", newSize, ResultCString(result));
                return;
            }
        }
    }

    updateIndexBufferData(buffer);
}


void VertexBuffer::bindVertexBuffer(AN::CommandBuffer *commandBuffer) {
    VK::CommandBuffer *vkCommand = (VK::CommandBuffer *)commandBuffer;

    UInt64 offsets[kMaxVertexStreams];
    UInt32 count = 0;
    for (int i = 0; i < kMaxVertexStreams; ++i) {
        if (_streams[i].channelMask && _streamModes[i] != kStreamModeDynamic) {
            offsets[count] = 0;
            ++count;
        } else {
            break;/// vertex stream must be continuous
        }
    }

    if (count > 0) {
        vkCommand->bindVertexBuffer(0, count, offsets, _VBStreams);
    }

    /// bind dynamic
    for (int i = count; i < kMaxVertexStreams; ++i) {
        if (_streams[i].channelMask && _streamModes[i] == kStreamModeDynamic) {
            vkCommand->bindVertexBuffer(0, 0, _dynVBStreams[_renderVersion % kMaxFrameInFlight][i].buffer);
            ++count;
        } else {
            break;/// vertex stream must be continuous
        }
    }

    if (count == 0) {
        AN_LOG(Error, "No vertex channel valid to bind");
    }
}

void VertexBuffer::bindIndexBuffer(AN::CommandBuffer *commandBuffer) {
    VK::CommandBuffer *vkCommand = (VK::CommandBuffer *)commandBuffer;
    if (_indicesDynamic) {
        vkCommand->bindIndexBuffer(VK_INDEX_TYPE_UINT16, 0, _dynIndexBuffer[_renderVersion % kMaxFrameInFlight].buffer);
    } else {
        vkCommand->bindIndexBuffer(VK_INDEX_TYPE_UINT16, 0, _indexBuffer);
    }
}

void VertexBuffer::drawIndexed(AN::CommandBuffer *commandBuffer, UInt32 indexCount, UInt32 indexOffset, UInt32 vertexOffset) {
//    _renderVersion = version;

    bindVertexBuffer(commandBuffer);
    bindIndexBuffer(commandBuffer);
    VK::CommandBuffer *vkCommand = (VK::CommandBuffer *)commandBuffer;
    vkCommand->drawIndexed(indexCount, indexOffset, vertexOffset);
}

void VertexBuffer::draw(AN::CommandBuffer *commandBuffer, UInt32 vertexCount) {
//    _renderVersion = version;

    bindVertexBuffer(commandBuffer);
    VK::CommandBuffer *vkCommand = (VK::CommandBuffer *)commandBuffer;
    vkCommand->draw(vertexCount);
}

}// namespace AN::VK
 //namespace AN::RC {


//bool VertexBuffer::init(uint64_t bytes) {
//
//    const RenderContext &context = GetRenderer().getRenderContext();
//
//    RC::BufferDescriptor bufferDescriptor{};
//    bufferDescriptor.size        = bytes;
//    bufferDescriptor.memoryUsage = MemoryUsage::AutoPreferDevice;
//    bufferDescriptor.bufferUsage = BufferUsageFlag::TransferDestination | BufferUsageFlag::VertexBuffer;
//
//    if (!vertexBuffer.init(context.device, bufferDescriptor)) {
//        return false;
//    }
//
//    return true;
//}
//
//
//bool VertexBuffer::initDynamic(uint64_t bytes, bool writeOnly) {
//
//    const RenderContext &context = GetRenderer().getRenderContext();
//    maxFrameInFlight             = context.maxFrameInFlight;
//    padSize                      = (bytes + 256 - 1) & ~(256 - 1);
//    _writeOnly                   = writeOnly;
//
//    bytes = padSize * maxFrameInFlight;
//
//    RC::BufferDescriptor bufferDescriptor{};
//    bufferDescriptor.size        = bytes;
//    bufferDescriptor.memoryUsage = MemoryUsage::Auto;
//    bufferDescriptor.bufferUsage = BufferUsageFlag::VertexBuffer;
//    if (writeOnly) {
//        bufferDescriptor.allocationFlag = AllocationFlag::HostAccessSequentialWrite;
//    } else {
//        bufferDescriptor.allocationFlag = AllocationFlag::HostAccessRandom;
//    }
//
//    if (!vertexBuffer.init(context.device, bufferDescriptor)) {
//        return false;
//    }
//
//    mappedBuffer = vertexBuffer.map();
//    return true;
//}
//
//void *VertexBuffer::content() {
//    if (maxFrameInFlight) {
//        /// dynamic vertex buffer
//        const RenderContext &context = GetRenderer().getRenderContext();
//        frameCount                   = context.frameCount;
//
//        if (!_writeOnly) {
//            vertexBuffer.invalidate();
//        }
//    }
//    return (char *) mappedBuffer + (frameCount % maxFrameInFlight) * padSize;
//}
//
//
//void VertexBuffer::deinit() {
//    vertexBuffer.deinit();
//}
//
//uint64_t VertexBuffer::getBufferOffset(uint64_t offset) {
//    if (maxFrameInFlight) {
//        /// dynamic vertex buffer
//
//        vertexBuffer.flush();
//
//        offset = offset + padSize * (frameCount % maxFrameInFlight);
//    }
//    return offset;
//}

//}