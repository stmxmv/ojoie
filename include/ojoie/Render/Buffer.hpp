//
// Created by Aleudillonam on 8/14/2022.
//

#ifndef OJOIE_RC_BUFFER_HPP
#define OJOIE_RC_BUFFER_HPP

#include "ojoie/Configuration/typedef.h"

namespace AN::RC {

class Device;

enum class BufferUsageFlag {
    None                = 0,
    TransferSource      = 1 << 0,
    TransferDestination = 1 << 1,
    VertexBuffer        = 1 << 2,
    IndexBuffer         = 1 << 3,
    UniformBuffer       = 1 << 4
};

enum class MemoryUsage {
    Auto,
    AutoPreferDevice,
    AutoPreferHost,
    Lazy
};

enum class AllocationFlag {
    None                      = 0,
    HostAccessSequentialWrite = 1 << 0,
    HostAccessRandom          = 1 << 1
};

struct BufferDescriptor {
    uint64_t size;
    BufferUsageFlag bufferUsage;
    MemoryUsage memoryUsage;
    AllocationFlag allocationFlag;
};

/// \brief VK::Buffer in vulkan
class Buffer : private NonCopyable {

    void *impl{};
public:

    Buffer();

    Buffer(Buffer &&other) noexcept
        : impl(other.impl) {
        other.impl = nullptr;
    }

    ~Buffer();

    bool init(Device &device, const BufferDescriptor &bufferDescriptor);

    void deinit();

    void *map();

    void unmap();

    /// \brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
    void flush();

    /// \brief invalidate memory if it is HOST_VISIBLE and not HOST_COHERENT
    void invalidate();
};

}

namespace AN {

template<>
struct enable_bitmask_operators<RC::BufferUsageFlag> : std::true_type {};

template<>
struct enable_bitmask_operators<RC::AllocationFlag> : std::true_type {};

}

#endif//OJOIE_RC_BUFFER_HPP
