//
// Created by aojoie on 4/20/2023.
//

#include "Render/private/vulkan/Semaphore.hpp"
#include "Render/private/vulkan/Device.hpp"

namespace AN::VK {


bool Semaphore::init(bool binary, UInt64 initialValue) {
    VkSemaphoreTypeCreateInfo timelineCreateInfo{};
    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.flags = 0; // flags is reserved by api

    if (!binary) {
        timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timelineCreateInfo.pNext = nullptr;
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue = initialValue;
        createInfo.pNext = &timelineCreateInfo;
    }

    VkResult result = vkCreateSemaphore(GetDevice().vkDevice(), &createInfo, NULL, &_semaphore);
    if (result != VK_SUCCESS) {
        AN_LOG(Error, "vulkan create semaphore error: %s", ResultCString(result));
    }
    return _semaphore != nullptr;
}

void Semaphore::deinit() {
    if (_semaphore) {
        vkDestroySemaphore(GetDevice().vkDevice(), _semaphore, nullptr);
        _semaphore = nullptr;
    }
}

void Semaphore::signal(UInt64 value) {
    VkSemaphoreSignalInfo signalInfo{};
    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
    signalInfo.semaphore = _semaphore;
    signalInfo.value = value;
    vkSignalSemaphore(GetDevice().vkDevice(), &signalInfo);
}

void Semaphore::wait(UInt64 value, UInt64 timeout) {
    VkSemaphoreWaitInfo waitInfo{};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    waitInfo.pNext = nullptr;
    waitInfo.flags = 0;
    waitInfo.semaphoreCount = 1;
    waitInfo.pSemaphores = &_semaphore;
    waitInfo.pValues = &value;
    vkWaitSemaphores(GetDevice().vkDevice(), &waitInfo, timeout);
}

UInt64 Semaphore::getValue() {
    UInt64 value;
    vkGetSemaphoreCounterValue(GetDevice().vkDevice(), _semaphore, &value);
    return value;
}

}