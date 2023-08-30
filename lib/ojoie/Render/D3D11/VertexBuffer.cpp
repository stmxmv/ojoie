//
// Created by aojoie on 5/24/2023.
//

#include "Render/private/D3D11/VertexBuffer.hpp"
#include "Render/private/D3D11/CommandBuffer.hpp"
#include "Render/private/D3D11/VertexInputLayouts.hpp"

#include <format>

namespace AN::D3D11 {

bool VertexBuffer::init() {
    return false;
}
void VertexBuffer::deinit() {
    for (auto &buffer : m_VBStreams) {
        buffer.Reset();
    }
    for (auto &buffer : m_StagingVB) {
        buffer.Reset();
    }
    m_IB.Reset();
    m_StagingIB.Reset();
}

void VertexBuffer::setVertexStreamMode(unsigned int stream, StreamMode mode) {
    if (_streamModes[stream] == mode) {
        return;
    }

    Super::setVertexStreamMode(stream, mode);

    for (auto &buffer : m_VBStreams) {
        buffer.Reset();
    }
}

void VertexBuffer::setIndicesDynamic(bool dynamic) {
    if (_indicesDynamic == dynamic) {
        /// no-ops
        return;
    }

    Super::setIndicesDynamic(dynamic);
    m_IB.Reset();
    m_StagingIB.Reset();
}

ComPtr<ID3D11Buffer> VertexBuffer::CreateStagingBuffer(size_t size) {
    ID3D11Device     *dev = GetD3D11Device();
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth           = size;
    desc.Usage               = D3D11_USAGE_STAGING;
    desc.BindFlags           = 0;
    desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;

    ComPtr<ID3D11Buffer> buffer;
    HRESULT              hr;
    D3D_ASSERT(hr, dev->CreateBuffer(&desc, NULL, &buffer));
    return buffer;
}

void VertexBuffer::updateVertexStream(const VertexBufferData &sourceData, unsigned int stream) {
    const StreamInfo &srcStream = sourceData.streams[stream];
    int               oldSize;
    oldSize = CalculateVertexStreamSize(_streams[stream], m_VertexCount);

    const int newSize = CalculateVertexStreamSize(srcStream, sourceData.vertexCount);

    _streams[stream] = srcStream;
    if (newSize == 0) {
        m_VBStreams[stream].Reset();
        m_StagingVB[stream].Reset();
        return;
    }

    const bool isDynamic  = (_streamModes[stream] == kStreamModeDynamic);
    const bool useStaging = !isDynamic;

    if (m_VBStreams[stream] == NULL || newSize != oldSize) {
        m_VBStreams[stream].Reset();
        m_StagingVB[stream].Reset();

        ID3D11Device     *dev = GetD3D11Device();
        D3D11_BUFFER_DESC desc;
        desc.ByteWidth = newSize;
        desc.Usage     = isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        //        if ( m_useForSO )
        //            desc.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
        desc.CPUAccessFlags      = isDynamic ? D3D11_CPU_ACCESS_WRITE : 0;
        desc.MiscFlags           = 0;
        desc.StructureByteStride = 0;
        HRESULT hr;
        D3D_ASSERT(hr, dev->CreateBuffer(&desc, NULL, &m_VBStreams[stream]));
        D3D11SetDebugName(m_VBStreams[stream].Get(), std::format("VertexBuffer-{}", newSize));

        if (useStaging) {
            m_StagingVB[stream] = CreateStagingBuffer(newSize);
            D3D11SetDebugName(m_StagingVB[stream].Get(), std::format("StagingVertexBuffer-{}", newSize));
        }
    }

    if (!sourceData.buffer)
        return;

    HRESULT hr;

    ID3D11Buffer *mapVB = NULL;
    D3D11_MAP     mapType;

    if (useStaging) {
        mapVB   = m_StagingVB[stream].Get();
        mapType = D3D11_MAP_WRITE;
    } else {
        mapVB   = m_VBStreams[stream].Get();
        mapType = D3D11_MAP_WRITE_DISCARD;
    }

    ANAssert(mapVB);

    ID3D11DeviceContext *ctx = GetD3D11Context();

    D3D11_MAPPED_SUBRESOURCE mapped;
    D3D_ASSERT(hr, ctx->Map(mapVB, 0, mapType, 0, &mapped));
    CopyVertexStream(sourceData, reinterpret_cast<UInt8 *>(mapped.pData), stream);
    ctx->Unmap(mapVB, 0);

    if (useStaging)
        ctx->CopyResource(m_VBStreams[stream].Get(), m_StagingVB[stream].Get());
}

void VertexBuffer::updateVertexData(const VertexBufferData &buffer) {
    for (unsigned stream = 0; stream < kMaxVertexStreams; stream++)
        updateVertexStream(buffer, stream);

    memcpy(m_ChannelInfo, buffer.channels, sizeof(m_ChannelInfo));
    m_VertexCount = std::max(buffer.vertexCount, m_VertexCount);
}

void VertexBuffer::updateIndexBufferData(const IndexBufferData &sourceData) {
    if (!sourceData.indices) {
        m_IBSize = 0;
        return;
    }

    ANAssert(m_IB);
    HRESULT hr;

    int size = sourceData.count * kVBOIndexSize;

    const D3D11_MAP mapType = _indicesDynamic ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
    ID3D11Buffer   *mapIB;
    if (_indicesDynamic)
        mapIB = m_IB.Get();
    else {
        if (!m_StagingIB)
            m_StagingIB = CreateStagingBuffer(m_IBSize);
        mapIB = m_StagingIB.Get();
    }

    ID3D11DeviceContext     *ctx = GetD3D11Context();
    D3D11_MAPPED_SUBRESOURCE mapped;
    D3D_ASSERT(hr, ctx->Map(mapIB, 0, mapType, 0, &mapped));

    memcpy(mapped.pData, sourceData.indices, size);

    ctx->Unmap(mapIB, 0);
    if (!_indicesDynamic)
        ctx->CopyResource(m_IB.Get(), m_StagingIB.Get());
}

void VertexBuffer::updateIndexData(const IndexBufferData &buffer) {
    int newSize = CalculateIndexBufferSize(buffer);

    // If we have old buffer, but need different size: delete old one
    if (newSize != m_IBSize) {
        m_IB.Reset();
        m_StagingIB.Reset();
    }
    // Create buffer if we need to
    if (!m_IB) {
        ID3D11Device     *dev = GetD3D11Device();
        D3D11_BUFFER_DESC desc;
        desc.ByteWidth           = newSize;
        desc.Usage               = _indicesDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        desc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags      = _indicesDynamic ? D3D11_CPU_ACCESS_WRITE : 0;
        desc.MiscFlags           = 0;
        desc.StructureByteStride = 0;
        HRESULT hr;
        D3D_ASSERT(hr, dev->CreateBuffer(&desc, NULL, &m_IB));
        D3D11SetDebugName(m_IB.Get(), std::format("IndexBuffer-{}", newSize));
    }

    m_IBSize = newSize;
    updateIndexBufferData(buffer);
}

void VertexBuffer::bindVertexStream(AN::CommandBuffer *_commandBuffer) {
    D3D11::CommandBuffer *commandBuffer = (D3D11::CommandBuffer *) _commandBuffer;
    ID3D11DeviceContext  *ctx           = commandBuffer->getContext();
    for (int s = 0; s < kMaxVertexStreams; ++s) {
        if (m_VBStreams[s]) {
            UINT offset = 0;
            UINT stride = _streams[s].stride;
            ctx->IASetVertexBuffers(s, 1, m_VBStreams[s].GetAddressOf(), &stride, &offset);
        }
    }

    ID3D11InputLayout *inputLayout = commandBuffer->getVertexInputLayout(m_ChannelInfo);
    ctx->IASetInputLayout(inputLayout);
}

void VertexBuffer::drawIndexed(AN::CommandBuffer *_commandBuffer,
                               UInt32 indexCount, UInt32 indexOffset, UInt32 vertexOffset) {

    bindVertexStream(_commandBuffer);
    D3D11::CommandBuffer *commandBuffer = (D3D11::CommandBuffer *) _commandBuffer;
    ID3D11DeviceContext  *ctx           = commandBuffer->getContext();
    ctx->IASetIndexBuffer(m_IB.Get(), DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _commandBuffer->drawIndexed(indexCount, indexOffset, vertexOffset);
}

void VertexBuffer::draw(AN::CommandBuffer *_commandBuffer, UInt32 vertexCount) {
    bindVertexStream(_commandBuffer);
    D3D11::CommandBuffer *commandBuffer = (D3D11::CommandBuffer *) _commandBuffer;
    ID3D11DeviceContext  *ctx           = commandBuffer->getContext();
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _commandBuffer->draw(vertexCount);
}

void DynamicVertexBuffer::initializeQuadsIB() {
    ANAssert(m_QuadsIB == nullptr);

    const int kMaxQuads = 65536 / 4 - 4;// so we fit into 16 bit indices, minus some more just in case

    UInt16 *data      = new UInt16[kMaxQuads * 6];
    UInt16 *ib        = data;
    UInt32  baseIndex = 0;
    for (int i = 0; i < kMaxQuads; ++i) {
        ib[0] = baseIndex + 1;
        ib[1] = baseIndex + 2;
        ib[2] = baseIndex;
        ib[3] = baseIndex + 2;
        ib[4] = baseIndex + 3;
        ib[5] = baseIndex;
        baseIndex += 4;
        ib += 6;
    }

    ID3D11Device     *dev = GetD3D11Device();
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth           = kMaxQuads * 6 * kVBOIndexSize;
    desc.Usage               = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags      = 0;
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA srData;
    srData.pSysMem          = data;
    srData.SysMemPitch      = 0;
    srData.SysMemSlicePitch = 0;
    HRESULT hr;
    D3D_ASSERT(hr, dev->CreateBuffer(&desc, &srData, &m_QuadsIB));
    delete[] data;

    D3D11SetDebugName(m_QuadsIB.Get(), "IndexBufferQuads");
}


bool DynamicVertexBuffer::getChunk(const ChannelInfoArray &channelInfo, UInt32 maxVertices,
                                   UInt32 maxIndices, RenderMode renderMode,
                                   void **outVB, void **outIB) {

    ANAssert(!m_LendedChunk);
    ANAssert(maxVertices < 65536 && maxIndices < 65536 * 3);
    ANAssert(outVB != NULL && maxVertices > 0);
    ANAssert((renderMode == kDrawIndexedQuads && (outIB != NULL && maxIndices > 0)) ||
             (renderMode == kDrawIndexedPoints && (outIB != NULL && maxIndices > 0)) ||
             (renderMode == kDrawIndexedLines && (outIB != NULL && maxIndices > 0)) ||
             (renderMode == kDrawIndexedTriangles && (outIB != NULL && maxIndices > 0)) ||
             (renderMode == kDrawQuads && (outIB == NULL && maxIndices == 0)));

    HRESULT hr;
    bool    success = true;

    m_LendedChunk                = true;
    memcpy(m_ChannelInfo, channelInfo, sizeof(channelInfo));
    m_LastRenderMode             = renderMode;

    if (maxVertices == 0)
        maxVertices = 8;

    m_LastChunkStride = 0;
    for (int i = 0; i < kShaderChannelCount; ++i) {
        if (channelInfo[i].isValid())
            m_LastChunkStride += GetChannelFormatSize(channelInfo[i].format) * channelInfo[i].dimension;
    }
    ID3D11Device        *dev = GetD3D11Device();
    ID3D11DeviceContext *ctx = GetD3D11Context();

    // -------- vertex buffer

    ANAssert(nullptr != outVB);
    UInt32 vbCapacity = maxVertices * m_LastChunkStride;
    // check if requested chunk is larger than current buffer
    if (vbCapacity > m_VBSize) {
        m_VBSize = vbCapacity * 2;// allocate more up front
        m_VB.Reset();
    }
    // allocate buffer if don't have it yet
    if (!m_VB) {
        D3D11_BUFFER_DESC desc;
        desc.ByteWidth           = m_VBSize;
        desc.Usage               = D3D11_USAGE_DYNAMIC;
        desc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags           = 0;
        desc.StructureByteStride = 0;
        hr                       = dev->CreateBuffer(&desc, NULL, &m_VB);
        if (FAILED(hr)) {
            AN_LOG(Error, "d3d11: failed to create dynamic vertex buffer of size %d [%lx]\n", m_VBSize, hr);
            success = false;
            *outVB  = NULL;
        } else {
            D3D11SetDebugName(m_VB.Get(), std::format("VertexBufferDynamic-{}", m_VBSize));
        }
    }

    // map, making sure the offset we lock is multiple of vertex stride
    if (m_VB) {
        m_VBUsedBytes = ((m_VBUsedBytes + (m_LastChunkStride - 1)) / m_LastChunkStride) * m_LastChunkStride;
        if (m_VBUsedBytes + vbCapacity > m_VBSize) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            hr = ctx->Map(m_VB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            if (FAILED(hr)) {
                AN_LOG(Error, "d3d11: failed to lock dynamic vertex buffer with discard [%lx]\n", hr);
                *outVB  = NULL;
                success = false;
            }
            *outVB        = mapped.pData;
            m_VBUsedBytes = 0;
        } else {
            D3D11_MAPPED_SUBRESOURCE mapped;
            hr = ctx->Map(m_VB.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped);
            if (FAILED(hr)) {
                AN_LOG(Error, "d3d11: failed to lock vertex index buffer, offset %i size %i [%lx]\n", m_VBUsedBytes, vbCapacity, hr);
                *outVB  = NULL;
                success = false;
            }
            *outVB = ((UInt8 *) mapped.pData) + m_VBUsedBytes;
        }
        m_LastChunkStartVertex = m_VBUsedBytes / m_LastChunkStride;
        ANAssert(m_LastChunkStartVertex * m_LastChunkStride == m_VBUsedBytes);
    }

    // -------- index buffer

    const bool indexed = (renderMode != kDrawQuads);
    if (success && maxIndices && indexed) {
        UInt32 ibCapacity = maxIndices * kVBOIndexSize;
        // check if requested chunk is larger than current buffer
        if (ibCapacity > m_IBSize) {
            m_IBSize = ibCapacity * 2;// allocate more up front
            m_IB.Reset();
        }
        // allocate buffer if don't have it yet
        if (!m_IB) {
            D3D11_BUFFER_DESC desc;
            desc.ByteWidth           = m_IBSize;
            desc.Usage               = D3D11_USAGE_DYNAMIC;
            desc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
            desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags           = 0;
            desc.StructureByteStride = 0;
            hr                       = dev->CreateBuffer(&desc, NULL, &m_IB);
            if (FAILED(hr)) {
                AN_LOG(Error, "d3d11: failed to create dynamic index buffer of size %d [%lx]\n", m_IBSize, hr);
                if (m_VB)
                    ctx->Unmap(m_VB.Get(), 0);
            }
            D3D11SetDebugName(m_IB.Get(), std::format("IndexBufferDynamic-{}", m_IBSize));
        }
        // lock it if we have IB created successfully
        if (m_IB) {
            if (m_IBUsedBytes + ibCapacity > m_IBSize) {
                D3D11_MAPPED_SUBRESOURCE mapped;
                hr = ctx->Map(m_IB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                if (FAILED(hr)) {
                    AN_LOG(Error, "d3d11: failed to lock dynamic index buffer with discard [%lx]\n", hr);
                    *outIB  = NULL;
                    success = false;
                    if (m_VB)
                        ctx->Unmap(m_VB.Get(), 0);
                }
                *outIB        = mapped.pData;
                m_IBUsedBytes = 0;
            } else {
                D3D11_MAPPED_SUBRESOURCE mapped;
                hr = ctx->Map(m_IB.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped);
                if (FAILED(hr)) {
                    AN_LOG(Error, "d3d11: failed to lock dynamic index buffer, offset %i size %i [%x]\n", m_IBUsedBytes, ibCapacity, hr);
                    *outIB  = NULL;
                    success = false;
                    if (m_VB)
                        ctx->Unmap(m_VB.Get(), 0);
                }
                *outIB = ((UInt8 *) mapped.pData) + m_IBUsedBytes;
            }
            m_LastChunkStartIndex = m_IBUsedBytes / 2;
        } else {
            *outIB  = NULL;
            success = false;
        }
    }

    if (!success)
        m_LendedChunk = false;

    return success;
}

void DynamicVertexBuffer::releaseChunk(UInt32 actualVertices, UInt32 actualIndices) {
    ANAssert(m_LendedChunk);
    ANAssert(m_LastRenderMode == kDrawIndexedQuads ||
             m_LastRenderMode == kDrawIndexedPoints ||
             m_LastRenderMode == kDrawIndexedLines ||
             actualIndices % 3 == 0);

    m_LendedChunk = false;

    const bool indexed = (m_LastRenderMode != kDrawQuads);

    m_LastChunkVertices = actualVertices;
    m_LastChunkIndices  = actualIndices;

    // unlock buffers
    ID3D11DeviceContext *ctx = GetD3D11Context();
    ctx->Unmap(m_VB.Get(), 0);
    if (indexed)
        ctx->Unmap(m_IB.Get(), 0);

    if (!actualVertices || (indexed && !actualIndices)) {
        for (int i = 0; i < kShaderChannelCount; ++i) {
            m_ChannelInfo[i].reset();
        }
        return;
    }

    UInt32 actualVBSize = actualVertices * m_LastChunkStride;
    m_VBUsedBytes += actualVBSize;
    UInt32 actualIBSize = actualIndices * kVBOIndexSize;
    m_IBUsedBytes += actualIBSize;
}

void DynamicVertexBuffer::drawChunk(AN::CommandBuffer *_commandBuffer,
                                    UInt32 indexCount,
                                    UInt32 indexOffset, UInt32 vertexOffset) {
    // just return if nothing to render
    if (!std::ranges::any_of(m_ChannelInfo, [](auto &&chan) { return chan.isValid(); }))
        return;

    if (indexCount == 0) return;

    D3D11::CommandBuffer *commandBuffer = (D3D11::CommandBuffer *) _commandBuffer;

    HRESULT hr;

    ANAssert(m_LastChunkStride);
    ANAssert(!m_LendedChunk);
    ANAssert(m_LastChunkIndices >= indexCount);

    ID3D11DeviceContext *ctx = GetD3D11Context();

    // setup VBO
    ANAssert(m_VB);
    UINT strides = m_LastChunkStride;
    UINT offsets = 0;
    ctx->IASetVertexBuffers(0, 1, m_VB.GetAddressOf(), &strides, &offsets);

    ID3D11InputLayout* inputLayout = commandBuffer->getVertexInputLayout(m_ChannelInfo);
    ctx->IASetInputLayout(inputLayout);

    int primCount = 0;
    if( m_LastRenderMode == kDrawQuads )
    {
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        // initialize quads index buffer if needed
        if (!m_QuadsIB)
            initializeQuadsIB();
        // if quads index buffer has valid data, draw with it
        if (m_QuadsIB) {
            ctx->IASetIndexBuffer(m_QuadsIB.Get(), DXGI_FORMAT_R16_UINT, 0);
            _commandBuffer->drawIndexed(indexCount / 4 * 6, 0, m_LastChunkStartVertex + vertexOffset);
            primCount = m_LastChunkVertices / 2;
        }
    } else if (m_LastRenderMode == kDrawIndexedLines) {
        ANAssert( m_IB );
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        ctx->IASetIndexBuffer (m_IB.Get(), DXGI_FORMAT_R16_UINT, 0);
        _commandBuffer->drawIndexed(indexCount, m_LastChunkStartIndex + indexOffset, m_LastChunkStartVertex + vertexOffset);
        primCount = m_LastChunkIndices/2;
    } else if (m_LastRenderMode == kDrawIndexedPoints) {
        ANAssert( m_IB );
        ctx->IASetIndexBuffer (m_IB.Get(), DXGI_FORMAT_R16_UINT, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        _commandBuffer->drawIndexed(indexCount, m_LastChunkStartIndex + indexOffset, m_LastChunkStartVertex + vertexOffset);
        primCount = m_LastChunkIndices;
    } else {
        ANAssert( m_IB );
        ctx->IASetIndexBuffer(m_IB.Get(), DXGI_FORMAT_R16_UINT, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _commandBuffer->drawIndexed(indexCount, m_LastChunkStartIndex + indexOffset, m_LastChunkStartVertex + vertexOffset);
        primCount = m_LastChunkIndices/3;
    }
}

}// namespace AN::D3D11