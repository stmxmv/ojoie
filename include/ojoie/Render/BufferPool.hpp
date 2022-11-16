//
// Created by Aleudillonam on 9/12/2022.
//

#ifndef OJOIE_RC_BUFFERPOOL_HPP
#define OJOIE_RC_BUFFERPOOL_HPP

#include <ojoie/Render/Buffer.hpp>
#include <ojoie/Template/Uninitialized.hpp>
#include <vector>

namespace AN::RC {

class BufferAllocation : private NonCopyable {
    void *impl{};
    AN::Uninitialized<Buffer> buffer;
public:

    BufferAllocation() = default;

    BufferAllocation(BufferAllocation &&other) noexcept : impl(other.impl), buffer(other.buffer) {
        other.impl = nullptr;
    }

    ~BufferAllocation();

    bool empty() const;

    void *map();

    void unmap();

    uint64_t getSize();

    uint64_t getOffset();

    Buffer &getBuffer();
};

class BufferBlock : private NonCopyable {
    void *impl{};
public:

    BufferBlock() = default;

    BufferBlock(BufferBlock &&other) : impl(other.impl) {
        other.impl = nullptr;
    }

    bool isValid() {
        return impl != nullptr;
    }

    BufferAllocation allocate(uint32_t allocate_size);

};

class BufferPool : private NonCopyable {
    void *impl{};
    bool shouldFree{};
public:

    BufferPool() = default;

    BufferPool(BufferPool &&other) : impl(other.impl) {
        other.impl = nullptr;
    }

    ~BufferPool() {
        deinit();
    }

    void deinit();

    BufferBlock bufferBlock(uint64_t minimum_size);

    void reset();

};

}

#endif//OJOIE_RC_BUFFERPOOL_HPP
