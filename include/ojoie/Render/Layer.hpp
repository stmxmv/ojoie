//
// Created by aojoie on 4/21/2023.
//

#ifndef OJOIE_LAYER_HPP
#define OJOIE_LAYER_HPP

#include <ojoie/Core/CGTypes.hpp>
#include <ojoie/Render/RenderTarget.hpp>

namespace AN {

class Window;

class Presentable {

public:

    virtual ~Presentable() = default;

    /// index between 0 ~ kMaxFrameInFlight - 1
    virtual UInt32 getFrameIndex() = 0;

    virtual RenderTarget *getRenderTarget() = 0;

};

class AN_API Layer {
    int index;
public:

    virtual ~Layer() = default;

    static Layer *Alloc();

    virtual bool init(Window *window) = 0;

    virtual Presentable *nextPresentable() = 0;

    virtual Size getSize() = 0;

    virtual void getDpiScale(float &x, float &y) { x = 1.f; y = 1.f; }

    virtual Window *getWindow() = 0;

    virtual void resize(const Size &size) = 0;

    virtual void setFullScreen(bool fullScreen) {}

    void setIndexInternal(int idx) { index = idx; }
    int getIndex() const { return index; }
};

}

#endif//OJOIE_LAYER_HPP
