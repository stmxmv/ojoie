//
// Created by Aleudillonam on 8/26/2022.
//

#include "Render/private/vulkan.hpp"
#include "Render/private/vulkan/Instance.hpp"
#include "Utility/Log.h"

#ifdef AN_DEBUG
#define ENABLE_VALIDATION_LAYERS
#endif

#define VK_CHECK(x)                                                                  \
	do                                                                               \
	{                                                                                \
		VkResult err = x;                                                            \
		if (err)                                                                     \
		{                                                                            \
			ANLog("Detected Vulkan error: " #x); \
			return false;                                                            \
		}                                                                            \
	} while (0)

namespace AN {

static bool validate_extensions(const std::vector<const char *> &required,
                                const std::vector<VkExtensionProperties> &available) {
    for (const auto *extension : required) {
        bool found = false;
        for (const auto &available_extension : available) {
            if (strcmp(available_extension.extensionName, extension) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            ANLog("Extension %s not found", extension);
            return false;
        }
    }

    return true;
}

static bool validate_layers(const std::vector<const char *> &required,
                            const std::vector<VkLayerProperties> &available) {
    for (const auto *layer : required) {
        bool found = false;
        for (const auto &available_layer : available) {
            if (strcmp(available_layer.layerName, layer) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            ANLog("Validation Layer %s not found", layer);
            return false;
        }
    }

    return true;
}

#if defined(AN_DEBUG) || defined(ENABLE_VALIDATION_LAYERS)

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {

    std::string severityString;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        severityString += "|VERBOSE";
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        severityString += "|INFO";
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        severityString += "|WARNING";
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        severityString += "|ERROR";
    }

    if (!severityString.empty()) {
        severityString.erase(0, 1);
    }

    std::string typeString;
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        typeString += "|GENERAL";
    }
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        typeString += "|VALIDATION";
    }
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        typeString += "|PERFORMANCE";
    }
    if (!typeString.empty()) {
        typeString.erase(0, 1);
    }

    ANLog("Validation Layer [%s] [%s] %s", severityString.c_str(), typeString.c_str(), pCallbackData->pMessage);

    /// The VK_TRUE value is reserved for use in layer development
    return VK_FALSE;
}

inline static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
    createInfo                 = {};
    createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}
#endif

bool VK::Instance::init(const InstanceDescriptor &descriptor) {
    VkResult result = volkInitialize();
    if (result) {
        ANLog("Failed to initialize volk %s", ResultCString(result));
        return false;
    }

    uint32_t instance_extension_count;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

    std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));

    extensions.assign(descriptor.requiredExtensions.begin(), descriptor.requiredExtensions.end());

#ifdef ENABLE_VALIDATION_LAYERS
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    // Try to enable headless surface extension if it exists
    if (descriptor.headless) {
        bool headless_extension = false;
        for (auto &available_extension : available_instance_extensions) {
            if (strcmp(available_extension.extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0) {
                headless_extension = true;
                ANLog("%s is available, enabling it", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                extensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
            }
        }
        if (!headless_extension) {
            ANLog("%s is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
        }
    }

    if (!validate_extensions(extensions, available_instance_extensions)) {
        ANLog("Required instance extensions are missing.");
        return false;
    }

    uint32_t instance_layer_count;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

    std::vector<VkLayerProperties> instance_layers(instance_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers.data()));

    std::vector<const char *> active_instance_layers(descriptor.requiredValidationLayers);

#ifdef ENABLE_VALIDATION_LAYERS
    active_instance_layers.push_back("VK_LAYER_KHRONOS_validation");

#ifdef AN_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    populateDebugMessengerCreateInfo(debugCreateInfo);
#endif
#endif

    if (validate_layers(active_instance_layers, instance_layers)) {
        for (const auto &layer : active_instance_layers) {
            ANLog("Enabled Validation Layer %s", layer);
        }
    } else {
        ANLog("Required validation layers are missing.");
        return false;
    }


    VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};

    app_info.pApplicationName   = descriptor.applicationName;
    app_info.applicationVersion = 0;
    app_info.pEngineName        = "Ojoie";
    app_info.engineVersion      = 0;
    app_info.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

    instance_info.pApplicationInfo = &app_info;

    instance_info.enabledExtensionCount   = (uint32_t)(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();

    instance_info.enabledLayerCount   = (uint32_t)(active_instance_layers.size());
    instance_info.ppEnabledLayerNames = active_instance_layers.data();

#if defined(AN_DEBUG) && defined(ENABLE_VALIDATION_LAYERS)
    instance_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
#endif
    // Create the Vulkan instance
    result = vkCreateInstance(&instance_info, nullptr, &handle);
    if (result != VK_SUCCESS) {
        ANLog("Could not create Vulkan instance");
        return false;
    }

    volkLoadInstance(handle);

#if defined(AN_DEBUG) && defined(ENABLE_VALIDATION_LAYERS)
    result = vkCreateDebugUtilsMessengerEXT(handle, &debugCreateInfo, nullptr, &debugUtilsMessenger);
    if (result != VK_SUCCESS) {
        ANLog("Could not create debug utils messenger.");
        return false;
    }
#endif

    // Querying valid physical devices on the machine
    uint32_t physical_device_count{0};
    VK_CHECK(vkEnumeratePhysicalDevices(handle, &physical_device_count, nullptr));

    if (physical_device_count < 1) {
        ANLog("Couldn't find a physical device that supports Vulkan.");
        return false;
    }

    gpus.resize(physical_device_count);

    VK_CHECK(vkEnumeratePhysicalDevices(handle, &physical_device_count, gpus.data()));


    return true;
}

void VK::Instance::deinit() {


    if (handle != VK_NULL_HANDLE) {
#if defined(AN_DEBUG) && defined(ENABLE_VALIDATION_LAYERS)
        vkDestroyDebugUtilsMessengerEXT(handle, debugUtilsMessenger, nullptr);
#endif
        vkDestroyInstance(handle, nullptr);

        handle = VK_NULL_HANDLE;
    }
}

VkPhysicalDevice VK::Instance::getPhysicalDevice() const {
    // Find a discrete GPU
    for (auto gpu : gpus) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(gpu, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            return gpu;
        }
    }

    // Otherwise just pick the first one
    ANLog("Couldn't find a discrete physical device, using integrated graphics");
    return gpus.at(0);
}

static VK::Instance gInstance;

void VK::InitializeInstance() {
    VK::InstanceDescriptor instanceDescriptor;
    instanceDescriptor.headless = false;
    instanceDescriptor.applicationName = "ojoie game instance";

    instanceDescriptor.requiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    instanceDescriptor.requiredExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

    /// TODO check if driver support below
//    instanceDescriptor.requiredExtensions.push_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);

#ifdef _WIN32
    instanceDescriptor.requiredExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else

#error "not implement"

#endif

    ANAssert(gInstance.init(instanceDescriptor));
}

void VK::DeallocInstance() {
    gInstance.deinit();
}

VK::Instance &VK::GetInstance() {
    return gInstance;
}

}