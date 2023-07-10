//
// Created by aojoie on 4/21/2023.
//

#ifndef OJOIE_VK_UNIFORMBUFFERS_HPP
#define OJOIE_VK_UNIFORMBUFFERS_HPP

#include <ojoie/Render/UniformBuffers.hpp>

#include <ojoie/Render/private/vulkan.hpp>
#include <ojoie/Threads/SpinLock.hpp>
#include <vector>

namespace AN::VK {


class UniformBuffers : public AN::UniformBuffers {

    struct UniformBufferInfo {
        VkBuffer      buffer;
        VmaAllocation allocation;
        UInt64        padSizePerFrame;
        UInt64        totalSize;
        UInt64        freeSizePerFrame;
        UInt64        version;
        void         *mappedData;
    };

    std::vector<UniformBufferInfo> oldBufferInfos;

    UniformBufferInfo currentBufferInfo;

    UInt64 bufferSizeUsedLastFrame{};
    UInt64 bufferSizeUsedPerFrame{};

    SpinLock spinLock;

    void allocateBuffer(UInt64 perFrameSize);

public:

    virtual bool init() override;

    virtual void deinit() override;

    virtual void update() override;

    /// thread-safe method, call in any thread
    UniformBufferAllocation allocate(UInt64 size);

};

UniformBuffers &GetUniformBuffers();

}// namespace AN::VK

#endif//OJOIE_VK_UNIFORMBUFFERS_HPP
