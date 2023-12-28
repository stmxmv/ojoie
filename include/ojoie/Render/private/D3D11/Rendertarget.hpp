//
// Created by aojoie on 5/23/2023.
//

#pragma once

#include <ojoie/Render/private/D3D11/Common.hpp>

#include <ojoie/Render/RenderTarget.hpp>

namespace AN::D3D11 {

class AN_API RenderTarget : public AN::RenderTargetImpl {


    ComPtr<ID3D11Texture2D> _texture;
    ComPtr<ID3D11RenderTargetView>   _rtv;
    ComPtr<ID3D11DepthStencilView>   _dsv;
    ComPtr<ID3D11ShaderResourceView> _srv;

public:

    virtual ~RenderTarget() override {
        deinit();
    }

    void bridgeSwapchain(ID3D11Texture2D *tex, ID3D11RenderTargetView *rtv) {
        _texture = tex;
        _rtv = rtv;
    }

    virtual bool init(const RenderTargetDescriptor &attachmentDescriptor) override;

    virtual void deinit() override;

    virtual Size getSize() const override;

    virtual UInt32 getMSAASamples() const override;

    ID3D11RenderTargetView *getRTV() const { return _rtv.Get(); }
    ID3D11ShaderResourceView *getSRV() const { return _srv.Get(); }
    ID3D11DepthStencilView *getDSV() const { return _dsv.Get(); }
    ID3D11Texture2D *getTexture() const { return _texture.Get(); }
};


}
