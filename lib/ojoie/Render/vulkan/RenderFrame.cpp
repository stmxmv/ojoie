//
// Created by Aleudillonam on 9/6/2022.
//

#include "Render/private/vulkan/RenderFrame.hpp"
#include "Render/private/vulkan/Device.hpp"

#define BUFFER_POOL_BLOCK_SIZE (256 * 1024)

namespace AN::VK {

bool RenderFrame::init(Device &device) {
    _device = &device;

    return semaphore_pool.init(device);
}

RenderFrame::RenderFrame(RenderFrame &&other) noexcept
    : semaphore_pool(std::move(other.semaphore_pool)),
      _device(other._device) {
}

void RenderFrame::deinit() {
    semaphore_pool.deinit();
}

void RenderFrame::reset(bool fence) {
    semaphore_pool.reset();
}

}// namespace AN::VK