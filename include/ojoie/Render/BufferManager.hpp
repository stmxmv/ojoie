//
// Created by Aleudillonam on 9/13/2022.
//

#ifndef OJOIE_RC_BUFFERMANAGER_HPP
#define OJOIE_RC_BUFFERMANAGER_HPP

#include <ojoie/Render/BufferPool.hpp>
#include <ojoie/Render/Buffer.hpp>

namespace AN::RC {

/// \brief a interface class to manage per frame data, it manage its own bufferPool and bufferBlocks
class BufferManager {
    void *impl{};
    bool shouldFree{};
public:

    ~BufferManager();

    void deinit();

    BufferAllocation buffer(BufferUsageFlag usage, uint64_t size);


};


}

#endif//OJOIE_RC_BUFFERMANAGER_HPP
