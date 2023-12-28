//
// Created by Aleudillonam on 8/14/2022.
//

#include "ojoie/Configuration/typedef.h"
#include "Render/VertexBuffer.hpp"

#ifdef OJOIE_USE_VULKAN
#include "vulkan/VertexBuffer.cpp"
#endif

#include "Render/RenderContext.hpp"
#include "Render/private/D3D11/VertexBuffer.hpp"

namespace AN {


void CopyVertexStream(const VertexBufferData &sourceData, void *buffer, unsigned stream) {
    ANAssert(stream < kMaxVertexStreams);

    if (!sourceData.buffer)
        return;

    const StreamInfo &info = sourceData.streams[stream];
    const UInt8      *src  = static_cast<const UInt8 *>(sourceData.buffer) + info.offset;
    size_t            size = CalculateVertexStreamSize(sourceData, stream);

    memcpy(buffer, src, size);
}


bool VertexBuffer::init() {
    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        impl = new VK::VertexBuffer();
#endif
    } else if (GetGraphicsAPI() == kGraphicsAPID3D11) {
        impl = new D3D11::VertexBuffer();
    }

    return impl != nullptr;
}

void VertexBuffer::deinit() {
    if (impl) {
        impl->deinit();
        delete impl;
        impl = nullptr;
    }
}

void VertexBuffer::setVertexStreamMode(unsigned int stream, StreamMode mode) {
    impl->setVertexStreamMode(stream, mode);
}

void VertexBuffer::setIndicesDynamic(bool dynamic) {
    impl->setIndicesDynamic(dynamic);
}

void VertexBuffer::updateVertexStream(const VertexBufferData &sourceData, unsigned int stream)
{
    impl->updateVertexStream(sourceData, stream);
}

void VertexBuffer::updateVertexData(const VertexBufferData &buffer) {
    impl->updateVertexData(buffer);
}

void VertexBuffer::updateIndexData(const IndexBufferData &buffer) {
    impl->updateIndexData(buffer);
}
void VertexBuffer::drawIndexed(CommandBuffer *commandBuffer, UInt32 indexCount,
                                   UInt32 indexOffset, UInt32 vertexOffset) {
    impl->drawIndexed(commandBuffer, indexCount, indexOffset, vertexOffset);
}

void VertexBuffer::draw(CommandBuffer *commandBuffer, UInt32 vertexCount) {
    impl->draw(commandBuffer, vertexCount);
}

static const int kChannelVertexSize[kShaderChannelCount] = {
    3 * sizeof (float),	// position
    3 * sizeof (float),	// normal
    1 * sizeof (UInt32),// color
    2 * sizeof (float),	// uv0
    2 * sizeof (float),	// uv1
    4 * sizeof (float),	// tangent
};


int GetDefaultChannelByteSize (int channelNum) {
    ANAssert( channelNum >= 0 && channelNum < kShaderChannelCount );
    return kChannelVertexSize[channelNum];
}

}// namespace AN
