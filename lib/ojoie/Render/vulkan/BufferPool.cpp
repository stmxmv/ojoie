//
// Created by Aleudillonam on 9/12/2022.
//

#include "Render/BufferPool.hpp"
#include "Render/private/vulkan/BufferPool.hpp"

#include "Template/Access.hpp"


namespace AN {

namespace  {

struct BufferImplTag : Access::TagBase<BufferImplTag> {};
struct BufferAllocationImplTag : Access::TagBase<BufferAllocationImplTag> {};
struct BufferBlockImplTag : Access::TagBase<BufferBlockImplTag> {};
struct BufferPoolImplTag : Access::TagBase<BufferPoolImplTag> {};

}

template struct Access::Accessor<BufferImplTag, &RC::Buffer::impl>;
template struct Access::Accessor<BufferAllocationImplTag, &RC::BufferAllocation::impl>;
template struct Access::Accessor<BufferBlockImplTag, &RC::BufferBlock::impl>;
template struct Access::Accessor<BufferPoolImplTag, &RC::BufferPool::impl>;

}
namespace AN::RC {



BufferAllocation::~BufferAllocation() {
    delete (VK::BufferAllocation *)impl;
    // we do not actually own the buffer, VK::BufferBlock does, set it nil to avoid destruction
    Access::set<BufferImplTag>((Buffer &)buffer, nullptr);
    impl = nullptr;
}

bool BufferAllocation::empty() const {
    VK::BufferAllocation *self = (VK::BufferAllocation *)impl;
    return self->empty();
}
void *BufferAllocation::map() {
    VK::BufferAllocation *self = (VK::BufferAllocation *)impl;
    return self->map();
}
void BufferAllocation::unmap() {
    VK::BufferAllocation *self = (VK::BufferAllocation *)impl;
    return self->unmap();
}
uint64_t BufferAllocation::getSize() {
    VK::BufferAllocation *self = (VK::BufferAllocation *)impl;
    return self->getSize();
}

uint64_t BufferAllocation::getOffset() {
    VK::BufferAllocation *self = (VK::BufferAllocation *)impl;
    return self->getOffset();
}

Buffer &BufferAllocation::getBuffer() {
    VK::BufferAllocation *self = (VK::BufferAllocation *)impl;

    Access::set<BufferImplTag>(buffer.get(), &self->getBuffer());

    return buffer.get();
}


BufferAllocation BufferBlock::allocate(uint32_t allocate_size) {
    VK::BufferBlock *self = (VK::BufferBlock *)impl;
    BufferAllocation allocation{};

    Access::set<BufferAllocationImplTag>(allocation, new VK::BufferAllocation(self->allocate(allocate_size)));

    return allocation;
}


void BufferPool::deinit() {
    if (shouldFree) {
        delete (VK::BufferPool *)impl;
        impl = nullptr;
        shouldFree = false;
    }
}

BufferBlock BufferPool::bufferBlock(uint64_t minimum_size) {
    VK::BufferPool *self = (VK::BufferPool *)impl;
    BufferBlock block{};

    Access::set<BufferBlockImplTag>(block, self->bufferBlock(minimum_size));

    return block;
}

void BufferPool::reset() {
    VK::BufferPool *self = (VK::BufferPool *)impl;
    self->reset();
}


}