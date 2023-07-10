//
// Created by Aleudillonam on 9/10/2022.
//

#include "Render/private/vulkan/CommandPool.hpp"
#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {

SubCommandPool::SubCommandPool(SubCommandPool &&other) noexcept
    : _device(other._device), handle(other.handle),
      primary_command_buffers(std::move(other.primary_command_buffers)),
      active_primary_command_buffer_count(other.active_primary_command_buffer_count),
      secondary_command_buffers(std::move(other.secondary_command_buffers)),
      active_secondary_command_buffer_count(other.active_secondary_command_buffer_count),
      _resetMode(other._resetMode) {
    other.handle = VK_NULL_HANDLE;
}


SubCommandPool::Ptr<CommandBuffer> SubCommandPool::allocateCommandBuffer(VkCommandBufferLevel level) {
    VkCommandBuffer             newBuffer;
    VkCommandBufferAllocateInfo allocate_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};

    allocate_info.commandPool        = handle;
    allocate_info.commandBufferCount = 1;
    allocate_info.level              = level;

    VkResult result = vkAllocateCommandBuffers(_device->vkDevice(), &allocate_info, &newBuffer);

    if (result != VK_SUCCESS) {
        ANLog("Failed to allocate command buffer code %d", result);
        return nullptr;
    }

    if (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        return std::make_unique<CommandBuffer>(*_device,
                                               _queueFamilyIndex,
                                               newBuffer,
                                               _resetMode,
                                               _primaryFencePool.newFence());
    }

    return std::make_unique<CommandBuffer>(*_device,
                                           _queueFamilyIndex,
                                           newBuffer,
                                           _resetMode,
                                           _secondaryFencePool.newFence());
}

bool SubCommandPool::init(Device &device, uint32_t queue_family_index,
                          uint32_t               maxCommandBufferNum,
                          CommandBufferResetMode reset_mode) {
    _device              = &device;
    _queueFamilyIndex    = queue_family_index;
    _resetMode           = reset_mode;
    _maxCommandBufferNum = maxCommandBufferNum;

    if (!_primaryFencePool.init(device.vkDevice())) {
        return false;
    }

    if (!_secondaryFencePool.init(device.vkDevice())) {
        return false;
    }

    VkCommandPoolCreateFlags flags;
    switch (reset_mode) {
        case kCommandBufferResetModeResetIndividually:
            flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            break;
        case kCommandBufferResetModeResetPool:
        default:
            flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            break;
    }

    VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};

    create_info.queueFamilyIndex = queue_family_index;
    create_info.flags            = flags;

    auto result = vkCreateCommandPool(_device->vkDevice(), &create_info, nullptr, &handle);

    if (result != VK_SUCCESS) {
        ANLog("Failed to create command pool");
        return false;
    }
    return true;
}

void SubCommandPool::deinit() {
    _primaryFencePool.deinit();
    _secondaryFencePool.deinit();

    primary_command_buffers.clear();
    secondary_command_buffers.clear();

    // Destroy command pool
    if (handle != VK_NULL_HANDLE) {
        vkDestroyCommandPool(_device->vkDevice(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}

VkResult SubCommandPool::reset() {
    VkResult result;

    _primaryFencePool.reset();
    _secondaryFencePool.reset();

    switch (_resetMode) {
        case kCommandBufferResetModeResetIndividually:
        case kCommandBufferResetModeResetPool: {
            result = vkResetCommandPool(_device->vkDevice(), handle, 0);

            if (result != VK_SUCCESS) {
                ANLog("vkResetCommandPool return %s", ResultCString(result));
                return result;
            }

            for (auto &commandBuffer : primary_command_buffers) {
                commandBuffer->reset();
                commandBuffer->setFenceInternal(_primaryFencePool.newFence());
            }

            for (auto &commandBuffer : secondary_command_buffers) {
                commandBuffer->reset();
                commandBuffer->setFenceInternal(_secondaryFencePool.newFence());
            }

            active_primary_command_buffer_count   = 0;
            active_secondary_command_buffer_count = 0;
            break;
        }
        default:
            throw std::runtime_error("Unknown reset mode for command pools");
    }



    return result;
}

CommandBuffer *SubCommandPool::newCommandBuffer(VkCommandBufferLevel level) {
    if (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        if (active_primary_command_buffer_count < primary_command_buffers.size()) {
            return primary_command_buffers.at(active_primary_command_buffer_count++).get();
        }

        if (primary_command_buffers.size() == _maxCommandBufferNum) {

            if (_resetMode == kCommandBufferResetModeResetIndividually) {

                /// just reset one commandBuffer
                if (VkResult result = _primaryFencePool.waitOne(); result != VK_SUCCESS) {
                    ANLog("fence_pool.wait() return %s", ResultCString(result));
                }

                for (const auto &commandBuffer : primary_command_buffers) {
                    if (commandBuffer->isCompleted()) {
                        commandBuffer->reset();
                        return commandBuffer.get();
                    }
                }

                ANAssert(false && "_fencePool.waitOne() error, should be at least one commandBuffer completed");

            } else {
                /// we must reset the whole pool
                waitAllCompleted();
                reset();
                ANAssert(active_primary_command_buffer_count == 0);
                return primary_command_buffers.at(active_primary_command_buffer_count++).get();
            }
        }

        auto commandBuffer = allocateCommandBuffer(level);

        if (!commandBuffer) return nullptr;

        primary_command_buffers.emplace_back(std::move(commandBuffer));

        active_primary_command_buffer_count++;

        return primary_command_buffers.back().get();
    }
    if (active_secondary_command_buffer_count < secondary_command_buffers.size()) {
        return secondary_command_buffers.at(active_secondary_command_buffer_count++).get();
    }

    if (secondary_command_buffers.size() == _maxCommandBufferNum) {
        if (_resetMode == kCommandBufferResetModeResetIndividually) {

            /// just reset one commandBuffer
            if (VkResult result = _secondaryFencePool.waitOne(); result != VK_SUCCESS) {
                ANLog("fence_pool.wait() return %s", ResultCString(result));
            }

            for (const auto &commandBuffer : secondary_command_buffers) {
                if (commandBuffer->isCompleted()) {
                    commandBuffer->reset();
                    return commandBuffer.get();
                }
            }

            ANAssert(false && "_fencePool.waitOne() error, should be at least one commandBuffer completed");

        } else {

            waitAllCompleted();
            reset();
            ANAssert(active_secondary_command_buffer_count == 0);
            return secondary_command_buffers.at(active_secondary_command_buffer_count++).get();
        }
    }

    auto commandBuffer = allocateCommandBuffer(level);

    if (!commandBuffer) return nullptr;

    secondary_command_buffers.emplace_back(std::move(commandBuffer));

    active_secondary_command_buffer_count++;

    return secondary_command_buffers.back().get();
}

void SubCommandPool::waitAllCompleted() {
    if (VkResult result = _primaryFencePool.wait(); result != VK_SUCCESS) {
        ANLog("fence_pool.wait() return %s", ResultCString(result));
    }
    if (VkResult result = _secondaryFencePool.wait(); result != VK_SUCCESS) {
        ANLog("fence_pool.wait() return %s", ResultCString(result));
    }
}

CommandPool::CommandPool(CommandPool &&other) noexcept
    : _device(other._device), subCommandPools(std::move(other.subCommandPools)),
      _maxCommandBufferNum(other._maxCommandBufferNum) {}

bool CommandPool::init(Device &device, uint32_t maxCommandBufferNum) {
    _device              = &device;
    _maxCommandBufferNum = maxCommandBufferNum;
    return true;
}

void CommandPool::deinit() {
    subCommandPools.clear();
}

void CommandPool::reset() {
    for (auto &&[_, subPool] : subCommandPools) {
        if (VkResult result = subPool.reset();
            result != VK_SUCCESS) {
            AN_LOG(Error, "reset vulkan commandPool return with error %s", ResultCString(result));
        }
    }
}

void CommandPool::waitAllCompleted() {
    for (auto &&[_, subPool] : subCommandPools) {
        subPool.waitAllCompleted();
    }
}

CommandBuffer *CommandPool::newCommandBuffer(uint32_t               queueFamilyIndex,
                                             CommandBufferResetMode resetMode,
                                             VkCommandBufferLevel   level) {
    struct SubCommandPoolInfo subCommandPoolInfo{ queueFamilyIndex, resetMode };

    if (auto it = subCommandPools.find(subCommandPoolInfo);
        it != subCommandPools.end()) {
        AN::VK::CommandBuffer *commandBuffer = it->second.newCommandBuffer(level);
        commandBuffer->beginRecord();
        return commandBuffer;
    }
    auto &pool = subCommandPools[subCommandPoolInfo];
    if (!pool.init(*_device, queueFamilyIndex, _maxCommandBufferNum, resetMode)) {
        return nullptr;
    }
    AN::VK::CommandBuffer *commandBuffer = pool.newCommandBuffer(level);
    commandBuffer->beginRecord();
    return commandBuffer;
}

AN::CommandBuffer *CommandPool::newCommandBuffer() {
    return newCommandBuffer(GetDevice().getGraphicsQueue().getFamilyIndex());
}

thread_local static std::unique_ptr<CommandPool> tCommandPool;

static constexpr UInt32 gCommandPoolBufferNum = 100;

void InitializeThreadCommandPool() {
    tCommandPool = std::make_unique<AN::VK::CommandPool>();
    ANAssert(tCommandPool->init(GetDevice(), gCommandPoolBufferNum));
}

void DeallocThreadCommandPool() {
    tCommandPool->deinit();
    tCommandPool->reset();
}

CommandPool &GetCommandPool() {
    if (tCommandPool == nullptr) {
        InitializeThreadCommandPool();
    }
    return *tCommandPool;
}

}// namespace AN::VK
