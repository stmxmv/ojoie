//
// Created by aojoie on 4/19/2023.
//

#ifndef OJOIE_COMMANDPOOL_HPP
#define OJOIE_COMMANDPOOL_HPP

#include <ojoie/Configuration/typedef.h>

namespace AN {

class CommandBuffer;

class AN_API CommandPool {
public:

    /// delete after used
    static CommandPool *newCommandPool();

    virtual ~CommandPool() = default;

    virtual CommandBuffer *newCommandBuffer() = 0;

    virtual void reset() = 0;

    virtual void waitAllCompleted() = 0;

    virtual void deinit() {}

};

AN_API CommandPool &GetCommandPool();

}

#endif//OJOIE_COMMANDPOOL_HPP
