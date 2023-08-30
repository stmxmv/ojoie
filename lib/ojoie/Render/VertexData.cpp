//
// Created by aojoie on 4/11/2023.
//

#include "Render/VertexData.hpp"
#include "Allocator/MemoryDefines.h"
#include "Utility/Utility.h"
#include <ojoie/Utility/Log.h>

namespace AN {


INSTANTIATE_TEMPLATE_TRANSFER(StreamInfo)
INSTANTIATE_TEMPLATE_TRANSFER(ChannelInfo)

static const uint8_t kVertexChannelFormatSizes[kChannelFormatCount] = {
    4,// kChannelFormatFloat
    2,// kChannelFormatFloat16
    4,// kChannelFormatColor
    1 // kChannelFormatByte
};

size_t GetChannelFormatSize(uint8_t format) {
    ANAssert(format < kChannelFormatCount);
    return kVertexChannelFormatSizes[format];
}

VertexStreamsLayout VertexDataInfo::kVertexStreamsDefault = { { kShaderChannelsAll, 0, 0, 0 } };

#define MAKE_CHANNEL(fmt, dim) VertexChannelsLayout::Channel(kChannelFormat##fmt, dim)
VertexChannelsLayout VertexDataInfo::kVertexChannelsDefault = { {
        MAKE_CHANNEL(Float, 3),// position
        MAKE_CHANNEL(Float, 3),// normal
        MAKE_CHANNEL(Color, 1),// color
        MAKE_CHANNEL(Float, 2),// texcoord0
        MAKE_CHANNEL(Float, 2),// texcoord1
        MAKE_CHANNEL(Float, 4) // tangent
} };

static void CopyChannels(size_t vertexCount, unsigned copyChannels,
                         const StreamInfoArray &srcStreams, const ChannelInfoArray &srcChannels, const UInt8 *srcData,
                         const StreamInfoArray &dstStreams, const ChannelInfoArray &dstChannels, UInt8 *dstData) {
    for (unsigned chan = copyChannels, i = 0; chan && (i < kShaderChannelCount); i++, chan >>= 1) {
        if (0 == (chan & 1))
            continue;

        const ChannelInfo &srcChannel = srcChannels[i];
        const ChannelInfo &dstChannel = dstChannels[i];

        const UInt8 *srcPtr    = srcData + srcChannel.calcOffset(srcStreams);
        UInt8       *dstPtr    = dstData + dstChannel.calcOffset(dstStreams);
        UInt8        srcStride = srcChannel.calcStride(srcStreams);
        UInt8        dstStride = dstChannel.calcStride(dstStreams);

        if (srcChannel.format == dstChannel.format) {
            size_t copySize = srcChannel.dimension * GetChannelFormatSize(srcChannel.format);
            switch (copySize) {
                case 4: {
                    for (size_t i = 0; i < vertexCount; ++i) {
                        *(reinterpret_cast<UInt32 *>(dstPtr) + 0) = *(reinterpret_cast<const UInt32 *>(srcPtr) + 0);
                        srcPtr += srcStride;
                        dstPtr += dstStride;
                    }
                    break;
                }
                case 8: {
                    for (size_t i = 0; i < vertexCount; ++i) {
                        *(reinterpret_cast<UInt32 *>(dstPtr) + 0) = *(reinterpret_cast<const UInt32 *>(srcPtr) + 0);
                        *(reinterpret_cast<UInt32 *>(dstPtr) + 1) = *(reinterpret_cast<const UInt32 *>(srcPtr) + 1);
                        srcPtr += srcStride;
                        dstPtr += dstStride;
                    }
                    break;
                }
                case 12: {
                    for (size_t i = 0; i < vertexCount; ++i) {
                        *(reinterpret_cast<UInt32 *>(dstPtr) + 0) = *(reinterpret_cast<const UInt32 *>(srcPtr) + 0);
                        *(reinterpret_cast<UInt32 *>(dstPtr) + 1) = *(reinterpret_cast<const UInt32 *>(srcPtr) + 1);
                        *(reinterpret_cast<UInt32 *>(dstPtr) + 2) = *(reinterpret_cast<const UInt32 *>(srcPtr) + 2);
                        srcPtr += srcStride;
                        dstPtr += dstStride;
                    }
                    break;
                }
                default: {
                    for (size_t i = 0; i < vertexCount; ++i) {
                        memcpy(dstPtr, srcPtr, copySize);
                        srcPtr += srcStride;
                        dstPtr += dstStride;
                    }
                    break;
                }
            }
        } else {
            //            ConvertCopyChannel(vertexCount, srcPtr, srcStride, srcChannel.format, srcChannel.dimension, dstPtr, dstStride, dstChannel.format, dstChannel.dimension);
            AN_LOG(Error, "currently not support different vertex format conversion");
        }
    }
}

VertexStreamsLayout VertexDataInfo::getStreamsLayout() const {
    VertexStreamsLayout result;
    for (int i = 0; i < kMaxVertexStreams; i++)
        result.channelMasks[i] = m_Streams[i].channelMask;
    return result;
}
VertexChannelsLayout VertexDataInfo::getChannelsLayout() const {
    VertexChannelsLayout result;
    for (int i = 0; i < kShaderChannelCount; i++) {
        result.channels[i] = VertexChannelsLayout::Channel(m_Channels[i].format, m_Channels[i].dimension);
    }
    return result;
}

void VertexDataInfo::updateStreams(unsigned int newChannelMask, size_t newVertexCount,
                                   const VertexStreamsLayout  &streams,
                                   const VertexChannelsLayout &channels) {
    m_VertexCount       = newVertexCount;
    m_CurrentChannels   = 0;
    m_VertexSize        = 0;
    size_t streamOffset = 0;
    for (int s = 0; s < kMaxVertexStreams; s++) {
        StreamInfo &stream = m_Streams[s];
        m_Streams[s].reset();
        stream.channelMask = streams.channelMasks[s] & newChannelMask;
        if (stream.channelMask == 0)
            continue;
        m_CurrentChannels |= stream.channelMask;
        for (int c = 0; c < kShaderChannelCount; c++) {
            if (stream.channelMask & (1 << c)) {
                ChannelInfo                         &channel    = m_Channels[c];
                const VertexChannelsLayout::Channel &srcChannel = channels.channels[c];
                channel.stream                                  = s;
                channel.offset                                  = stream.stride;
                channel.format                                  = srcChannel.format;
                channel.dimension                               = srcChannel.dimension;
                stream.stride += channel.dimension * GetChannelFormatSize(channel.format);
            }
        }
        streamOffset  = AlignStreamSize(streamOffset);
        stream.offset = streamOffset;
        streamOffset += stream.stride * newVertexCount;
        m_VertexSize += stream.stride;
    }
    for (int c = 0; c < kShaderChannelCount; c++) {
        // Reset channels that were removed
        if (!(m_CurrentChannels & (1 << c)))
            m_Channels[c].reset();
    }
    m_DataSize = streamOffset;
}

bool VertexDataInfo::conformsToStreamsLayout(const VertexStreamsLayout &streams) const {
    for (int i = 0; i < kMaxVertexStreams; i++) {
        // Fail if we have a channel that's not in the layout
        if (m_Streams[i].channelMask & ~streams.channelMasks[i])
            return false;
    }
    return true;
}

bool VertexDataInfo::conformsToChannelsLayout(const VertexChannelsLayout &channels) const {
    for (int i = 0; i < kShaderChannelCount; i++) {
        if (m_Channels[i].isValid()) {
            const VertexChannelsLayout::Channel &channel = channels.channels[i];
            if (m_Channels[i].format != channel.format ||
                m_Channels[i].dimension != channel.dimension)
                return false;
        }
    }
    return true;
}

VertexDescriptor VertexDataInfo::getVertexDescriptor(std::span<const ShaderVertexInput> shaderVertexInput) {
    VertexDescriptor vertexDescriptor{};

    static std::unordered_map<std::string_view, ShaderChannel> semanticChannelMap{
        { "POSITION", kShaderChannelVertex },
        { "NORMAL", kShaderChannelNormal },
        { "COLOR", kShaderChannelColor },
        { "TEXCOORD0", kShaderChannelTexCoord0 },
        { "TEXCOORD1", kShaderChannelTexCoord1 },
        { "TANGENT", kShaderChannelTangent }
    };

    for (const StreamInfo &streamInfo : m_Streams) {
        if (streamInfo.channelMask) {
            /// has channel
            VertexLayoutDescriptor vertexLayoutDescriptor{};
            vertexLayoutDescriptor.stride = streamInfo.stride;

            /// TODO default use per vertex, may make it parameter in future
            vertexLayoutDescriptor.stepFunction = kVertexStepFunctionPerVertex;

            vertexDescriptor.layouts.push_back(vertexLayoutDescriptor);
        }
    }

    for (const ShaderVertexInput &input : shaderVertexInput) {
        VertexAttributeDescriptor vertexAttributeDescriptor{};
        vertexAttributeDescriptor.location  = input.location;
        vertexAttributeDescriptor.format    = input.format;
        vertexAttributeDescriptor.dimension = input.dimension;

        auto it = semanticChannelMap.find(input.semantic);

        if (it == semanticChannelMap.end()) {
            AN_LOG(Error, "shader has unknown semantic %s", input.semantic.c_str());
            /// TODO try to do something here
        } else {
            ShaderChannel      channel        = it->second;
            const ChannelInfo &channelInfo    = m_Channels[channel];
            vertexAttributeDescriptor.offset  = channelInfo.offset;
            vertexAttributeDescriptor.binding = channelInfo.stream;
        }

        vertexDescriptor.attributes.push_back(vertexAttributeDescriptor);
    }

    return vertexDescriptor;
}

VertexDataInfo::VertexDataInfo()
    : m_Data(),
      m_DataSize(),
      m_VertexCount(),
      m_VertexSize(),
      m_CurrentChannels() {}

VertexData::~VertexData() {
    dealloc();
}

void VertexData::dealloc() {
    ANSafeFree(m_Data);
}

void VertexData::resize(size_t vertexCount, unsigned int channelMask,
                        const VertexStreamsLayout  &streams,
                        const VertexChannelsLayout &channels) {
    ChannelInfoArray srcChannels;
    StreamInfoArray  srcStreams;
    memcpy(srcChannels, m_Channels, sizeof(srcChannels));
    memcpy(srcStreams, m_Streams, sizeof(srcStreams));
    uint32_t srcChannelMask = m_CurrentChannels;
    uint32_t srcVertexCount = m_VertexCount;
    UInt8   *srcData        = m_Data;

    updateStreams(channelMask, vertexCount, streams, channels);

    // In case the streams and channels don't change, simply reallocate the buffer and return
    // Note that this will rarely be true with multiple streams since the stream offsets change
    if (m_Data && CompareMemory(srcChannels, m_Channels) && CompareMemory(srcStreams, m_Streams)) {
        m_Data = (UInt8 *) AN_REALLOC_ALIGNED(m_Data, VertexData::GetAllocateDataSize(m_DataSize), kVertexDataAlign);
        return;
    }

    m_Data = (UInt8 *) AN_MALLOC_ALIGNED(VertexData::GetAllocateDataSize(m_DataSize), kVertexDataAlign);
    // copy over the old data
    if (srcData) {
        unsigned copyChannels = srcChannelMask & m_CurrentChannels;
        size_t   toCopyCount  = std::min<size_t>(srcVertexCount, m_VertexCount);
        CopyChannels(toCopyCount, copyChannels, srcStreams, srcChannels, srcData, m_Streams, m_Channels, m_Data);
        AN_FREE(srcData);
    }
}

template<typename Coder>
void VertexData::transfer(Coder &coder) {
    coder.transfer(m_CurrentChannels, "m_CurrentChannels", kHideInEditorMask);
    coder.transfer(m_VertexSize, "m_VertexSize", kHideInEditorMask);
    coder.transfer(m_VertexCount, "m_VertexCount", kHideInEditorMask);

    std::vector<ChannelInfo> channels;
    std::vector<StreamInfo>  streams;

    if constexpr (Coder::IsEncoding()) {
        channels.resize(kShaderChannelCount);
        streams.resize(kMaxVertexStreams);
        std::copy(m_Channels, m_Channels + kShaderChannelCount, channels.begin());
        std::copy(m_Streams, m_Streams + kMaxVertexStreams, streams.begin());
    }

    coder.transfer(channels, "m_Channels", kHideInEditorMask);
    coder.transfer(streams, "m_Streams", kHideInEditorMask);

    if constexpr (Coder::IsDecoding()) {
        if (channels.size() == kShaderChannelCount)
            std::copy(channels.begin(), channels.begin() + kShaderChannelCount, m_Channels);
        if (streams.size() == kMaxVertexStreams)
            std::copy(streams.begin(), streams.begin() + kMaxVertexStreams, m_Streams);
        else
            std::fill(m_Streams, m_Streams + kMaxVertexStreams, StreamInfo());

        UInt32 channelsInStreams = 0;
        for (int i = 0; i < kMaxVertexStreams; i++)
            channelsInStreams |= m_Streams[i].channelMask;
        if (channelsInStreams)
            updateStreams(channelsInStreams, m_VertexCount, getStreamsLayout(), getChannelsLayout());
        else
            updateStreams(m_CurrentChannels, m_VertexCount, kVertexStreamsDefault, kVertexChannelsDefault);
    }

    coder.transferTypeless(m_DataSize, "m_DataSize", kHideInEditorMask);
    if constexpr (Coder::IsDecoding()) {
        ANSafeFree(m_Data);
        m_Data = (UInt8 *) AN_MALLOC_ALIGNED(GetAllocateDataSize(m_DataSize), kVertexDataAlign);
    }
    coder.transferTypelessData(m_Data, m_DataSize);
}

INSTANTIATE_TEMPLATE_TRANSFER(VertexData)


}// namespace AN