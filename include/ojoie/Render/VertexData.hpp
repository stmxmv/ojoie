//
// Created by aojoie on 4/10/2023.
//

#ifndef OJOIE_VERTEXDATA_HPP
#define OJOIE_VERTEXDATA_HPP

#include <ojoie/Configuration/typedef.h>
#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Template/StrideIterator.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Serialize/SerializeMetaFlags.h>

namespace AN {

/// different stream will use different layout and vertex buffer
typedef struct StreamInfo {

    uint32_t channelMask;
    uint32_t offset;
    uint8_t  stride;

    // We use default constructors instead of memset()
    StreamInfo() : channelMask(), offset(), stride() {}

    void reset() { *this = StreamInfo(); }

    bool operator==(const StreamInfo &) const = default;

    DECLARE_SERIALIZE_NO_IDPTR(StreamInfo)

} StreamInfoArray[kMaxVertexStreams];

template<typename Coder>
void StreamInfo::transfer(Coder &coder) {
    coder.transfer(channelMask, "channelMask", kHideInEditorMask);
    coder.transfer(offset, "offset", kHideInEditorMask);
    coder.transfer(stride, "stride", kHideInEditorMask);
}

struct VertexStreamsLayout {
    uint32_t channelMasks[kMaxVertexStreams];
};

typedef struct alignas(4) ChannelInfo {
    uint8_t stream;
    uint8_t offset;
    uint8_t format;
    uint8_t dimension;
    
    enum { kInvalidDimension = 0 };

    // We use default constructors instead of memset()
    ChannelInfo() : stream(0), offset(0), format(0), dimension(kInvalidDimension) {}

    uint32_t calcOffset(const StreamInfoArray &streams) const { return streams[stream].offset + offset; }
    uint32_t calcStride(const StreamInfoArray &streams) const { return streams[stream].stride; }
    bool isValid() const { return (kInvalidDimension != dimension); }
    void reset() { *this = ChannelInfo(); }

    bool operator == (const ChannelInfo& rhs) const = default;

    DECLARE_SERIALIZE_NO_IDPTR(ChannelInfo)

} ChannelInfoArray [kShaderChannelCount];

template<typename Coder>
void ChannelInfo::transfer(Coder &coder) {
    coder.transfer(stream, "stream", kHideInEditorMask);
    coder.transfer(offset, "offset", kHideInEditorMask);
    coder.transfer(format, "format", kHideInEditorMask);
    coder.transfer(dimension, "dimension", kHideInEditorMask);
}

struct VertexChannelsLayout {
    struct Channel {
        Channel(uint8_t fmt, uint8_t dim) noexcept : format(fmt), dimension(dim) {}
        Channel() : format(0), dimension(0) {}
        uint8_t format;
        uint8_t dimension;
    };
    Channel channels[kShaderChannelCount];
};

class AN_API VertexDataInfo {

public:
    enum {
        kVertexDataAlign = 32,
        kVertexStreamAlign = 16,
        kVertexDataPadding = 16
    };

    static VertexStreamsLayout kVertexStreamsDefault;
    static VertexChannelsLayout kVertexChannelsDefault;


protected:
    ChannelInfoArray m_Channels;
    StreamInfoArray	m_Streams;

    size_t m_VertexSize; // must match m_CurrentChannels
    uint8_t * m_Data;

    // The following are being serialized. Their size must match in both 32 and 64 bit platforms
    uint32_t m_CurrentChannels; // kShaderChannel bitmask
    uint32_t m_VertexCount;
    size_t   m_DataSize;

    static size_t AlignStreamSize(size_t size) { return (size + (kVertexStreamAlign-1)) & ~(kVertexStreamAlign-1); }

public:
    VertexDataInfo();

    bool hasChannel (ShaderChannel shaderChannelIndex) const {
        ANAssert ((m_Channels[shaderChannelIndex].dimension != 0) == (((m_CurrentChannels & (1 << shaderChannelIndex)) != 0)));
        return m_Channels[shaderChannelIndex].dimension != 0;
    }

    const StreamInfo* getStreams() const { return m_Streams; }
    const StreamInfo& getStream(int index) const { return m_Streams[index]; }

    const ChannelInfo* getChannels() const { return m_Channels; }
    const ChannelInfo& getChannel(int index) const { return m_Channels[index]; }

    VertexStreamsLayout getStreamsLayout() const;
    VertexChannelsLayout getChannelsLayout() const;

    size_t getDataSize () const { return m_DataSize; }
    UInt8* getDataPtr () const { return m_Data; }
    size_t getVertexSize () const { return m_VertexSize; }
    size_t getVertexCount () const { return m_VertexCount; }

    unsigned getChannelMask () const { return m_CurrentChannels; }

    size_t getChannelOffset (unsigned channel) const { return m_Channels[channel].calcOffset(m_Streams); }
    size_t getChannelStride (unsigned channel) const { return m_Channels[channel].calcStride(m_Streams); }

    bool conformsToStreamsLayout(const VertexStreamsLayout& streams) const;
    bool conformsToChannelsLayout(const VertexChannelsLayout& channels) const;

    template<typename T>
    StrideIterator<T> MakeStrideIterator (ShaderChannel shaderChannelIndex) const {
        ANAssert (shaderChannelIndex < kShaderChannelCount);
        void* p = m_Data + getChannelOffset(shaderChannelIndex);
        return hasChannel (shaderChannelIndex) ? StrideIterator<T> (p, getChannelStride(shaderChannelIndex)) : StrideIterator<T>(nullptr, getChannelStride(shaderChannelIndex));
    }

    template<typename T>
    StrideIterator<T> MakeEndIterator (ShaderChannel shaderChannelIndex) const {
        T* end = GetEndPointer<T>(shaderChannelIndex);
        return StrideIterator<T> (end, getChannelStride(shaderChannelIndex));
    }

    template<typename T>
    T* GetEndPointer (ShaderChannel shaderChannelIndex) const {
        ANAssert (shaderChannelIndex < kShaderChannelCount);
        void* p = hasChannel (shaderChannelIndex) ? (m_Data + getChannelOffset(shaderChannelIndex) + m_VertexCount * getChannelStride (shaderChannelIndex)) : nullptr;
        return reinterpret_cast<T*> (p);
    }

    void updateStreams(unsigned newChannelMask, size_t newVertexCount,
                       const VertexStreamsLayout& streams = kVertexStreamsDefault,
                       const VertexChannelsLayout& channels = kVertexChannelsDefault);

    VertexDescriptor getVertexDescriptor(std::span<const ShaderVertexInput> shaderVertexInput);
};

class AN_API VertexData : public VertexDataInfo{

public:

    DECLARE_SERIALIZE_NO_IDPTR(VertexData)

    ~VertexData();

    void resize (size_t vertexCount, unsigned channelMask,
                const VertexStreamsLayout& streams = kVertexStreamsDefault,
                const VertexChannelsLayout& channels = kVertexChannelsDefault);

    void dealloc();

    static size_t GetAllocateDataSize(size_t accessibleBufferSize) { return accessibleBufferSize + kVertexDataPadding; }

};

size_t GetChannelFormatSize(uint8_t format);

}// namespace AN

template<>
struct std::hash<AN::VertexStreamsLayout> {
    size_t operator()(const AN::VertexStreamsLayout &layout) const {
        size_t result = 0;
        for (UInt32 mask : layout.channelMasks) {
            AN::Math::hash_combine(result, mask);
        }
        return result;
    }
};

template<>
struct std::hash<AN::VertexChannelsLayout> {
    size_t operator()(const AN::VertexChannelsLayout &layout) const {
        size_t result = 0;
        for (const auto &channel : layout.channels) {
            UInt16 hash = (UInt16)channel.format + (((UInt16)channel.dimension) << 8);
            AN::Math::hash_combine(result, hash);
        }
        return result;
    }
};


#endif//OJOIE_VERTEXDATA_HPP
