//
// Created by Aleudillonam on 8/29/2022.
//

#ifndef OJOIE_COMMANDPOOL_HPP
#define OJOIE_COMMANDPOOL_HPP

namespace AN::VK {

enum class CommandBufferResetMode {
    ResetPool,
    ResetIndividually,
    AlwaysAllocate,
};

class CommandPool : private NonCopyable {

    VkDevice _device;

    VkCommandPool handle{};

    uint32_t _queueFamilyIndex;

    std::vector<VkCommandBuffer> primary_command_buffers;

    uint32_t active_primary_command_buffer_count{};

    std::vector<VkCommandBuffer> secondary_command_buffers;

    uint32_t active_secondary_command_buffer_count{};

    CommandBufferResetMode _resetMode;

    VkResult resetCommandBuffer(VkCommandBuffer commandBuffer) {
        VkResult result = VK_SUCCESS;

        if (_resetMode == CommandBufferResetMode::ResetIndividually) {
            result = vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        }

        return result;
    }

    VkResult reset_command_buffers() {
        VkResult result = VK_SUCCESS;

        for (auto &cmd_buf : primary_command_buffers) {
            result = resetCommandBuffer(cmd_buf);

            if (result != VK_SUCCESS) {
                return result;
            }
        }

        active_primary_command_buffer_count = 0;

        for (auto &cmd_buf : secondary_command_buffers) {
            result = resetCommandBuffer(cmd_buf);

            if (result != VK_SUCCESS) {
                return result;
            }
        }

        active_secondary_command_buffer_count = 0;

        return result;
    }

    VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        VkCommandBuffer newBuffer;
        VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };

        allocate_info.commandPool        = handle;
        allocate_info.commandBufferCount = 1;
        allocate_info.level              = level;

        VkResult result = vkAllocateCommandBuffers(_device, &allocate_info, &newBuffer);

        if (result != VK_SUCCESS) {
            ANLog("Failed to allocate command buffer code %d", result);
            return nullptr;
        }
        return newBuffer;
    }

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

    bool init(VkDevice device, uint32_t queue_family_index,
              CommandBufferResetMode reset_mode = CommandBufferResetMode::ResetPool) {
        _device      = device;
        _resetMode   = reset_mode;
        _queueFamilyIndex = queue_family_index;

        VkCommandPoolCreateFlags flags;
        switch (reset_mode) {
            case CommandBufferResetMode::ResetIndividually:
            case CommandBufferResetMode::AlwaysAllocate:
                flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                break;
            case CommandBufferResetMode::ResetPool:
            default:
                flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
                break;
        }

        VkCommandPoolCreateInfo create_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};

        create_info.queueFamilyIndex = queue_family_index;
        create_info.flags            = flags;

        auto result = vkCreateCommandPool(_device, &create_info, nullptr, &handle);

        if (result != VK_SUCCESS) {
            ANLog("Failed to create command pool");
            return false;
        }
        return true;
    }

    void deinit() {
        primary_command_buffers.clear();
        secondary_command_buffers.clear();

        // Destroy command pool
        if (handle != VK_NULL_HANDLE) {
            vkDestroyCommandPool(_device, handle, nullptr);
            handle = VK_NULL_HANDLE;
        }
    }

    VkResult reset() {
        VkResult result = VK_SUCCESS;
        switch (_resetMode) {
            case CommandBufferResetMode::ResetIndividually: {
                result = reset_command_buffers();

                break;
            }
            case CommandBufferResetMode::ResetPool: {
                result = vkResetCommandPool(_device, handle, 0);

                if (result != VK_SUCCESS)
                {
                    return result;
                }

                result = reset_command_buffers();

                break;
            }
            case CommandBufferResetMode::AlwaysAllocate: {
                primary_command_buffers.clear();
                active_primary_command_buffer_count = 0;

                secondary_command_buffers.clear();
                active_secondary_command_buffer_count = 0;

                break;
            }
            default:
                throw std::runtime_error("Unknown reset mode for command pools");
        }

        return result;
    }


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
