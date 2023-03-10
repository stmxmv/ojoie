//
// Created by Aleudillonam on 8/29/2022.
//

#ifndef OJOIE_COMMANDPOOL_HPP
#define OJOIE_COMMANDPOOL_HPP

#include "Render/private/vulkan.hpp"

namespace AN::VK {

enum class CommandBufferResetMode {
    ResetPool,
    ResetIndividually
};

class Device;

class CommandPool : private NonCopyable {

    Device *_device;

    VkCommandPool handle{};

    uint32_t _queueFamilyIndex;

    std::vector<VkCommandBuffer> primary_command_buffers;

    uint32_t active_primary_command_buffer_count{};

    std::vector<VkCommandBuffer> secondary_command_buffers;

    uint32_t active_secondary_command_buffer_count{};

    CommandBufferResetMode _resetMode;

    VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

public:

    CommandPool() = default;

    CommandPool(CommandPool &&other) noexcept
        : _device(other._device), handle(other.handle), _queueFamilyIndex(other._queueFamilyIndex),
          primary_command_buffers(std::move(other.primary_command_buffers)),
          active_primary_command_buffer_count(other.active_primary_command_buffer_count),
          secondary_command_buffers(std::move(other.secondary_command_buffers)),
          active_secondary_command_buffer_count(other.active_secondary_command_buffer_count),
          _resetMode(other._resetMode) {

        other.handle = VK_NULL_HANDLE;
    }

    ~CommandPool() {
        deinit();
    }

    bool init(Device &device, uint32_t queue_family_index,
              CommandBufferResetMode reset_mode = CommandBufferResetMode::ResetPool);

    void deinit();

    Device &getDevice() const {
        return *_device;
    }

    uint32_t getQueueFamilyIndex() const {
        return _queueFamilyIndex;
    }

    /// \brief reset the whole command pool, thus reset all allocated command buffers
    VkResult reset();


    VkCommandBuffer newCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        if (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
            if (active_primary_command_buffer_count < primary_command_buffers.size()) {
                return primary_command_buffers.at(active_primary_command_buffer_count++);
            }

            primary_command_buffers.emplace_back(allocateCommandBuffer(level));

            active_primary_command_buffer_count++;

            return primary_command_buffers.back();
        } else {
            if (active_secondary_command_buffer_count < secondary_command_buffers.size()) {
                return secondary_command_buffers.at(active_secondary_command_buffer_count++);
            }

            secondary_command_buffers.emplace_back(allocateCommandBuffer(level));

            active_secondary_command_buffer_count++;

            return secondary_command_buffers.back();
        }
    }

    CommandBufferResetMode getResetMode() const {
        return _resetMode;
    }

    VkCommandPool vkCommandPool() const {
        return handle;
    }
};


}

#endif//OJOIE_COMMANDPOOL_HPP
