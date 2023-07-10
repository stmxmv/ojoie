//
// Created by aojoie on 4/22/2023.
//

#ifndef OJOIE_VK_VERTEXBUFFER_HPP
#define OJOIE_VK_VERTEXBUFFER_HPP


#include <ojoie/Render/private/vulkan.hpp>
#include <ojoie/Render/private/vulkan/Semaphore.hpp>

namespace AN::VK {


/// lazily allocated until you update
/// abstraction of vertex buffer and index buffer, index will use UInt16 no matter what
/// when use dynamic vertexBuffer, you cannot update after draw in the same frame, this will override it
class VertexBuffer : public VertexBufferImpl {
    typedef VertexBufferImpl Super;

    struct BufferInfo {
        VkBuffer buffer;
        VmaAllocation allocation;
        UInt32 version;
    };

    VkBuffer      _VBStreams[kMaxVertexStreams];
    VmaAllocation _VBAllocation[kMaxVertexStreams];

    VkBuffer      _stagingVB[kMaxVertexStreams];
    VmaAllocation _stagingVBAllocation[kMaxVertexStreams];

    Semaphore _stagingVBSemaphore[kMaxVertexStreams];
    UInt64    _stagingVBSemaphoreValue[kMaxVertexStreams];

    VkBuffer      _indexBuffer;
    VmaAllocation _indexAllocation;

    VkBuffer      _stagingIndexBuffer;
    VmaAllocation _stagingIndexAllocation;

    Semaphore _stagingIndexSemaphore;
    UInt64    _stagingIndexSemaphoreValue;


    BufferInfo _dynVBStreams[kMaxFrameInFlight][kMaxVertexStreams];
    BufferInfo _dynIndexBuffer[kMaxFrameInFlight];


    UInt32 _currentVersion;
    UInt32 _renderVersion;

    int _indexBufferSize;
    int _vertexCount;

    int _dynIndexBufferSize[kMaxFrameInFlight];
    int _dynVertexCount[kMaxFrameInFlight];

    std::vector<BufferInfo> _oldBuffers;

    void setVersionAndCleanup(UInt32 version);

    void pendingDestroyVertexBufferStream(int stream, int index);
    void pendingDestroyIndexBuffer(int index);

    UInt32 frameIndex() const { return _currentVersion % kMaxFrameInFlight; }

public:
    VertexBuffer();

    virtual bool init() override { return true; }

    virtual void deinit() override;

    static void createStagingBuffer(VkBuffer *outBuffer, VmaAllocation *outAllocation, int size);

    void cleanupOldBuffers();

    void updateIndexBufferData(const IndexBufferData &sourceData);

    void updateVertexStream(const VertexBufferData &sourceData, unsigned stream);

    virtual void setVertexStreamMode(unsigned int stream, StreamMode mode) override;

    virtual void setIndicesDynamic(bool dynamic) override;

    virtual void updateVertexData(const VertexBufferData &buffer) override;
    virtual void updateIndexData(const IndexBufferData &buffer) override;

    void bindVertexBuffer(AN::CommandBuffer *commandBuffer);

    void bindIndexBuffer(AN::CommandBuffer *commandBuffer);

    /// bind buffer and draw indexed
    virtual void drawIndexed(AN::CommandBuffer *commandBuffer, UInt32 indexCount, UInt32 indexOffset, UInt32 vertexOffset) override;

    /// bind buffer and draw
    virtual void draw(AN::CommandBuffer *commandBuffer, UInt32 vertexCount) override;
};

}// namespace AN::VK


#endif//OJOIE_VK_VERTEXBUFFER_HPP
