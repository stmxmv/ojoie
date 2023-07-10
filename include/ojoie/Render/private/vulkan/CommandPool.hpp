//
// Created by Aleudillonam on 8/29/2022.
//

#ifndef OJOIE_VK_COMMANDPOOL_HPP
#define OJOIE_VK_COMMANDPOOL_HPP

#include <ojoie/Threads/SpinLock.hpp>
#include "ojoie/Render/private/vulkan.hpp"
#include <ojoie/Render/private/vulkan/CommandBuffer.hpp>
#include <ojoie/Render/private/vulkan/FencePool.hpp>
#include <ojoie/Render/CommandPool.hpp>

namespace AN::VK {

class Device;

struct SubCommandPoolInfo {
    uint32_t                 queueFamilyIndex;
    CommandBufferResetMode   flags;

    bool operator== (const SubCommandPoolInfo &) const = default;
};

}// namespace AN::VK

template<>
struct std::hash<AN::VK::SubCommandPoolInfo> {
    size_t operator() (const AN::VK::SubCommandPoolInfo &info) const {
        return (size_t) info.queueFamilyIndex | ((size_t) info.flags << 32);
    }
};

namespace AN::VK {


/// SubCommandPool is related to a single queue family index and specific reset mode
class SubCommandPool : private NonCopyable {

    template<typename T>
    using Ptr = std::unique_ptr<T>;

    Device                         *_device;
    FencePool                       _primaryFencePool;
    FencePool                       _secondaryFencePool;
    VkCommandPool                   handle{};
    std::vector<Ptr<CommandBuffer>> primary_command_buffers;
    std::vector<Ptr<CommandBuffer>> secondary_command_buffers;

    uint32_t active_primary_command_buffer_count{};
    uint32_t active_secondary_command_buffer_count{};

    uint32_t _maxCommandBufferNum;

    uint32_t               _queueFamilyIndex;
    CommandBufferResetMode _resetMode;

    Ptr<CommandBuffer> allocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

public:
    SubCommandPool() = default;

    SubCommandPool(SubCommandPool &&other) noexcept;

    ~SubCommandPool() {
        deinit();
    }

    bool init(Device &device, uint32_t queue_family_index,
              uint32_t               maxCommandBufferNum = std::numeric_limits<uint32_t>::max(),
              CommandBufferResetMode reset_mode          = kCommandBufferResetModeResetPool);

    void deinit();

    /// \brief reset the whole command pool, thus reset all allocated command buffers
    VkResult reset();

    CommandBuffer *newCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    CommandBufferResetMode getResetMode() const { return _resetMode; }

    VkCommandPool vkCommandPool() const { return handle; }

    void waitAllCompleted();
};

/// abstraction of VkCommandPool that manages all kinds of commandPools
class CommandPool : public AN::CommandPool, private NonCopyable {

    Device                                                *_device;
    std::unordered_map<SubCommandPoolInfo, SubCommandPool> subCommandPools;

    uint32_t _maxCommandBufferNum;

public:
    CommandPool() = default;

    CommandPool(CommandPool &&other) noexcept;

    virtual ~CommandPool() override {
        deinit();
    }

    bool init(Device &device, uint32_t maxCommandBufferNum = std::numeric_limits<uint32_t>::max());

    void deinit();

    Device &getDevice() const { return *_device; }

    virtual void reset() override;

    virtual void waitAllCompleted() override;

    /// only this method can be called in multi-thread parallelly
    /// if resetMode is individual, you may use unlimited pool size,
    /// otherwise, the pool may allocate the previous used commandBuffer as the new one, which is not thread safe
    CommandBuffer *newCommandBuffer(uint32_t               queueFamilyIndex,
                                    CommandBufferResetMode resetMode = kCommandBufferResetModeResetPool,
                                    VkCommandBufferLevel   level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    virtual AN::CommandBuffer *newCommandBuffer() override;
};

void InitializeThreadCommandPool();

void DeallocThreadCommandPool();

/// global command pool has a limit size, always use reset mode as resetPool
CommandPool &GetCommandPool();

}// namespace AN::VK

#endif//OJOIE_VK_COMMANDPOOL_HPP
