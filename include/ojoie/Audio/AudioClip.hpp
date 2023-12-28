//
// Created by aojoie on 8/31/2023.
//

#pragma once

#include <ojoie/Object/Object.hpp>

namespace AN {


class AN_API AudioClip : public Object {

    struct Impl;
    Impl *impl;

    AN_CLASS(AudioClip, Object)

public:

    explicit AudioClip(ObjectCreationMode mode);

    virtual bool init(const char *name, bool stream);
    virtual void dealloc() override;
    float getLength();
    UInt32 getSamples();

    void *getSound();
};

}
