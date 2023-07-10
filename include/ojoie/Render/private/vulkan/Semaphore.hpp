//
// Created by aojoie on 4/20/2023.
//

#ifndef OJOIE_VK_SEMAPHORE_HPP
#define OJOIE_VK_SEMAPHORE_HPP

#include <ojoie/Render/private/vulkan.hpp>

namespace AN::VK {

class Semaphore : private NonCopyable {

    VkSemaphore _semaphore{};

public:

    Semaphore() = default;

    Semaphore(Semaphore &&other) noexcept : _semaphore(other._semaphore) {
        other._semaphore = nullptr;
    }

    ~Semaphore() {
        deinit();
    }

    ///  initialValue is ignored when binary is true
    bool init(bool binary, UInt64 initialValue = 0);

    void deinit();

    bool isValid() const { return _semaphore != nullptr; }

    /// this method can only be called when binary is false
    void signal(UInt64 value);

    void wait(UInt64 value, UInt64 timeout = std::numeric_limits<UInt64>::max());

    /// this method can only be called when binary is false
    UInt64 getValue();

    VkSemaphore vkSemaphore() const { return _semaphore; }

};



}

#endif//OJOIE_VK_SEMAPHORE_HPP
