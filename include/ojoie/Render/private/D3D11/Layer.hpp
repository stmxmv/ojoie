//
// Created by aojoie on 5/23/2023.
//

#pragma once

#include <ojoie/Core/private/win32/Window.hpp>
#include <ojoie/Render/Layer.hpp>
#include <ojoie/Render/private/D3D11/Common.hpp>
#include <ojoie/Render/private/D3D11/Rendertarget.hpp>
#include <ojoie/Render/RenderTarget.hpp>
#include <ojoie/Object/ObjectPtr.hpp>

namespace AN::D3D11 {

struct Presentable : public AN::Presentable {

    ObjectPtr<AN::RenderTarget> _renderTarget;
    ComPtr<IDXGISwapChain> _swapChain;

    UInt32 imageIndex = 0; // this is always 0 for D3D11

    virtual UInt32 getFrameIndex() override {
        return imageIndex;
    }

    virtual AN::RenderTarget *getRenderTarget() override {
        return _renderTarget.get();
    }

};


class Layer : public AN::Layer {

    WIN::Window           *_window;
    ComPtr<IDXGISwapChain1> _swapChain;

    RenderTarget _rendertarget;
    Presentable _presentable;

public:
    virtual ~Layer() override;

    virtual bool init(Window *window) override;

    virtual AN::Presentable *nextPresentable() override {
        return &_presentable;
    }

    virtual Size getSize() override;

    virtual void resize(const Size &size) override;

    virtual void getDpiScale(float &x, float &y) override;

    virtual Window *getWindow() override;

    virtual void setFullScreen(bool fullScreen) override;
};


}// namespace AN::D3D11