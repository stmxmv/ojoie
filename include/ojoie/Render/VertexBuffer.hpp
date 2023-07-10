//
// Created by Aleudillonam on 8/14/2022.
//

#ifndef OJOIE_VERTEXBUFFER_HPP
#define OJOIE_VERTEXBUFFER_HPP

#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Render/VertexData.hpp>

namespace AN {
struct RenderContext;

class CommandBuffer;

// Whats the access pattern for modifying vertices?
enum StreamMode {
    kStreamModeNoAccess = 0,
    kStreamModeDynamic,
    kStreamModeCount
};

struct VertexBufferData {
    StreamInfoArray  streams;
    ChannelInfoArray channels;
    const UInt8     *buffer;
    int              bufferSize;
    int              vertexCount;
};

/// indices must UInt16
struct IndexBufferData {
    const void *indices;
    int         count;
};


class VertexBufferImpl {
protected:
    ChannelInfoArray     m_ChannelInfo;
    StreamMode      _streamModes[kMaxVertexStreams];
    bool            _indicesDynamic;
    StreamInfoArray _streams;

public:
    virtual ~VertexBufferImpl() = default;

    VertexBufferImpl() {
        for (int i = 0; i < kMaxVertexStreams; ++i) {
            _streamModes[i] = kStreamModeNoAccess;
            _streams[i].reset();
        }
        _indicesDynamic = false;
    }

    virtual bool init()   = 0;
    virtual void deinit() = 0;

    /// after change mode, you need to re-upload vertex data
    virtual void setVertexStreamMode(unsigned stream, StreamMode mode) {
        _streamModes[stream] = mode;
    }

    /// after change mode, you need to re-upload index data
    virtual void setIndicesDynamic(bool dynamic) {
        _indicesDynamic = dynamic;
    }

    /// upload data

    virtual void updateVertexData(const VertexBufferData &buffer) = 0;

    virtual void updateIndexData(const IndexBufferData &buffer) = 0;

    /// bind buffer and draw indexed
    virtual void drawIndexed(AN::CommandBuffer *commandBuffer, UInt32 indexCount,
                             UInt32 indexOffset, UInt32 vertexOffset) = 0;

    /// bind buffer and draw
    virtual void draw(AN::CommandBuffer *commandBuffer, UInt32 vertexCount) = 0;
};

static inline int CalculateVertexStreamSize(const StreamInfo &stream, int vertexCount) {
    return stream.stride * vertexCount;
}

static inline int CalculateVertexStreamSize(const VertexBufferData &buffer, unsigned stream) {
    ANAssert(stream < kMaxVertexStreams);
    return CalculateVertexStreamSize(buffer.streams[stream], buffer.vertexCount);
}

static constexpr int kVBOIndexSize = sizeof(UInt16);

inline int CalculateIndexBufferSize(const IndexBufferData &buffer) {
    int size = 0;
    if (buffer.indices)
        size += buffer.count * kVBOIndexSize;
    return size;
}

AN_API int GetDefaultChannelByteSize(int channelNum);

AN_API void CopyVertexStream(const VertexBufferData &sourceData, void *buffer, unsigned stream);


class AN_API VertexBuffer : private NonCopyable {

    VertexBufferImpl *impl;

public:
    VertexBuffer() = default;

    VertexBuffer(VertexBuffer &&other) noexcept
        : impl(other.impl) {
        other.impl = nullptr;
    }

    ~VertexBuffer() {
        deinit();
    }

    VertexBufferImpl *getImpl() const { return impl; }

    bool init();

    void deinit();

    void setVertexStreamMode(unsigned stream, StreamMode mode);

    void setIndicesDynamic(bool dynamic);

    void updateVertexData(const VertexBufferData &buffer);

    void updateIndexData(const IndexBufferData &buffer);

    void drawIndexed(AN::CommandBuffer *commandBuffer, UInt32 indexCount,
                     UInt32 indexOffset, UInt32 vertexOffset);

    void draw(AN::CommandBuffer *commandBuffer, UInt32 vertexCount);
};

enum RenderMode {
    kDrawIndexedTriangles,		// arbitrary triangle list
    kDrawQuads,					// no index buffer, four vertices per quad
    kDrawIndexedLines,			// arbitraty line list
    kDrawIndexedPoints,			// arbitraty point lits
    kDrawIndexedQuads,			// arbitraty quad lits
    kDrawTriangles,				// no index buffer triangle list
};

class AN_API DynamicVertexBuffer {

protected:
    ChannelInfoArray     m_ChannelInfo;
    UInt32	m_LastChunkStride, m_LastChunkVertices, m_LastChunkIndices;
    RenderMode	m_LastRenderMode;
    bool	m_LendedChunk;

public:

    virtual ~DynamicVertexBuffer() = default;

    virtual bool getChunk(const ChannelInfoArray &channelInfo,
                          UInt32 maxVertices,
                          UInt32 maxIndices, RenderMode mode, void** outVB, void** outIB) = 0;

    virtual void releaseChunk( UInt32 actualVertices, UInt32 actualIndices ) = 0;

    virtual void drawChunk(AN::CommandBuffer *commandBuffer,
                           UInt32 indexCount,
                           UInt32 indexOffset, UInt32 vertexOffset) = 0;

};

}// namespace AN

#endif//OJOIE_VERTEXBUFFER_HPP
