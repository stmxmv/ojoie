//
// Created by Aleudillonam on 8/26/2022.
//

#ifndef OJOIE_INSTANCE_HPP
#define OJOIE_INSTANCE_HPP

#include "ojoie/Render/private/vulkan.hpp"
#include "ojoie/Configuration/typedef.h"
#include <vector>

namespace AN::VK {

struct InstanceDescriptor {
    const char *applicationName;
    std::vector<const char *> requiredExtensions;
    std::vector<const char *> requiredValidationLayers;
    bool headless;
};

class Instance : private NonCopyable {

    VkInstance handle{}; // VkInstance handle

    /**
	 * @brief The enabled extensions
	 */
    std::vector<const char *> extensions;

#if AN_DEBUG
    /**
	 * @brief The debug report callback
	 */
    VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };
#endif

    /**
	 * @brief The physical devices found on the machine
	 */
    std::vector<VkPhysicalDevice> gpus;

public:

    Instance() = default;

    Instance(Instance &&other) noexcept
        : handle(other.handle), extensions(std::move(other.extensions)),

#if AN_DEBUG
          debugUtilsMessenger(other.debugUtilsMessenger),
#endif
          gpus(std::move(other.gpus)) {

        other.handle = VK_NULL_HANDLE;

#if AN_DEBUG
        other.debugUtilsMessenger = VK_NULL_HANDLE;
#endif
    }

    ~Instance() {
        deinit();
    }

    bool init(const InstanceDescriptor &descriptor);

    void deinit();

    VkPhysicalDevice getPhysicalDevice() const;

    VkInstance vKInstance() const {
        return handle;
    }
};

void InitializeInstance();
void DeallocInstance();
Instance &GetInstance();

}

#endif//OJOIE_INSTANCE_HPP
