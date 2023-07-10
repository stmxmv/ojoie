//
// Created by aojoie on 5/24/2023.
//

#pragma once
#include <ojoie/Render/VertexBuffer.hpp>
#include <ojoie/Render/private/D3D11/Common.hpp>

namespace AN::D3D11 {

class VertexBuffer : public VertexBufferImpl {
    typedef VertexBufferImpl Super;

    int                  m_VertexCount;
    ComPtr<ID3D11Buffer> m_VBStreams[kMaxVertexStreams];
    ComPtr<ID3D11Buffer> m_StagingVB[kMaxVertexStreams];
    ComPtr<ID3D11Buffer> m_IB;
    ComPtr<ID3D11Buffer> m_StagingIB;
    int		m_IBSize;

    static ComPtr<ID3D11Buffer> CreateStagingBuffer(size_t size);

    void updateIndexBufferData (const IndexBufferData& sourceData);
    void updateVertexStream(const VertexBufferData &sourceData, unsigned stream);

    void bindVertexStream(AN::CommandBuffer *commandBuffer);

public:
    virtual bool init() override;

    virtual void deinit() override;

    virtual void setVertexStreamMode(unsigned int stream, StreamMode mode) override;

    virtual void setIndicesDynamic(bool dynamic) override;

    virtual void updateVertexData(const VertexBufferData &buffer) override;

    virtual void updateIndexData(const IndexBufferData &buffer) override;

    virtual void drawIndexed(AN::CommandBuffer *commandBuffer,
                             UInt32 indexCount, UInt32 indexOffset, UInt32 vertexOffset) override;

    virtual void draw(AN::CommandBuffer *commandBuffer, UInt32 vertexCount) override;
};


class DynamicVertexBuffer : public AN::DynamicVertexBuffer {

    ComPtr<ID3D11Buffer> m_QuadsIB;

    UInt32	m_VBSize;
    UInt32	m_VBUsedBytes;
    UInt32	m_IBSize;
    UInt32	m_IBUsedBytes;

    ComPtr<ID3D11Buffer>	m_VB;
    ComPtr<ID3D11Buffer>	m_IB;

    UInt32 m_LastChunkStartVertex;
    UInt32 m_LastChunkStartIndex;

    void initializeQuadsIB();

public:

    virtual bool getChunk(const ChannelInfoArray &channelInfo,
                          UInt32 maxVertices,
                          UInt32 maxIndices, RenderMode mode, void** outVB, void** outIB ) override;

    virtual void releaseChunk( UInt32 actualVertices, UInt32 actualIndices ) override;

    virtual void drawChunk(AN::CommandBuffer *commandBuffer,
                           UInt32 indexCount,
                           UInt32 indexOffset, UInt32 vertexOffset) override;

    ~DynamicVertexBuffer() {
        deinit();
    }

    ID3D11Buffer* getQuadsIB() {
        if (!m_QuadsIB)
            initializeQuadsIB();
        return m_QuadsIB.Get();
    }

    void deinit() {
        m_VB.Reset();
        m_IB.Reset();
        m_QuadsIB.Reset();
    }

};

}// namespace AN::D3D11
