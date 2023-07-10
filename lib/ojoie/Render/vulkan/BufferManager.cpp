//
// Created by Aleudillonam on 9/13/2022.
//

#include "Render/BufferManager.hpp"
#include "Render/private/vulkan/BufferManager.hpp"

#include "Template/Access.hpp"

namespace AN {

namespace  {

struct BufferAllocationImplTag : Access::TagBase<BufferAllocationImplTag> {};

}

template struct Access::Accessor<BufferAllocationImplTag, &RC::BufferAllocation::impl>;

}

namespace AN::RC {

inline static VkBufferUsageFlags toVkBufferUsageFlags(BufferUsageFlag flag) {
    VkBufferUsageFlags ret{};

    if ((flag & BufferUsageFlag::TransferSource) != BufferUsageFlag::None) {
        ret |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    if ((flag & BufferUsageFlag::TransferDestination) != BufferUsageFlag::None) {
        ret |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    if ((flag & BufferUsageFlag::VertexBuffer) != BufferUsageFlag::None) {
        ret |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }

    if ((flag & BufferUsageFlag::IndexBuffer) != BufferUsageFlag::None) {
        ret |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    if ((flag & BufferUsageFlag::UniformBuffer) != BufferUsageFlag::None) {
        ret |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }

    return ret;
}

BufferManager::~BufferManager() {
    if (shouldFree) {
        delete (VK::BufferManager *)impl;
        impl = nullptr;
        shouldFree = false;
    }
}

void BufferManager::deinit() {
    if (impl) {
        VK::BufferManager *self = (VK::BufferManager *)impl;
        self->deinit();
    }
}

BufferAllocation BufferManager::buffer(BufferUsageFlag usage, uint64_t size) {
    VK::BufferManager *self = (VK::BufferManager *)impl;
    BufferAllocation allocation{};

    Access::set<BufferAllocationImplTag>(allocation, new VK::BufferAllocation(self->buffer(toVkBufferUsageFlags(usage), size)));

    return allocation;
}

}