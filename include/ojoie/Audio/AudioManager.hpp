//
// Created by aojoie on 8/31/2023.
//

#pragma once

#include <ojoie/Export/Export.h>

namespace AN {

class AN_API AudioManager {

public:

    virtual ~AudioManager() = default;

    virtual bool init() = 0;

    virtual void deinit() = 0;

    virtual void update(float deltaTime) = 0;

};

AN_API AudioManager &GetAudioManager();

}
