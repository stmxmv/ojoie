//
// Created by Aleudillonam on 8/3/2023.
//

#pragma once

#include <ojoie/Camera/Camera.hpp>
#include <ojoie/Render/CommandBuffer.hpp>

namespace AN {

class PhysicsManager {
    struct Impl;
    Impl *impl;

    PhysicsManager();

    ~PhysicsManager();
public:

    static PhysicsManager &GetSharedPhysics();

    bool init();

    void deinit();

    void update(float deltaTime);

    void renderVisualization(CommandBuffer *commandBuffer);
};

static inline PhysicsManager &GetPhysicsManager() {
    return PhysicsManager::GetSharedPhysics();
}

}