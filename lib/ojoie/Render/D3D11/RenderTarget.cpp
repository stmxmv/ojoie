//
// Created by aojoie on 5/23/2023.
//

#include "Render/private/D3D11/Rendertarget.hpp"
#include "Render/private/D3D11/Device.hpp"



namespace AN::D3D11 {

bool RenderTarget::init(const AttachmentDescriptor &attachmentDescriptor) {

    HRESULT hr;

    D3D11_TEXTURE2D_DESC desc;
    desc.Width = attachmentDescriptor.width;
    desc.Height = attachmentDescriptor.height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = toDXGIFormat(attachmentDescriptor.format);
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    if (attachmentDescriptor.samples > 1) {
        UINT msaaQualityLevels;
        D3D_ASSERT(hr, GetD3D11Device()->CheckMultisampleQualityLevels(
                               desc.Format,
                               attachmentDescriptor.samples,
                               &msaaQualityLevels
                               ));

        if (msaaQualityLevels > 0) {
            desc.SampleDesc.Count = attachmentDescriptor.samples;
            desc.SampleDesc.Quality = msaaQualityLevels - 1;
        }
    }


    desc.Usage = D3D11_USAGE_DEFAULT;


    if (attachmentDescriptor.format == kRTFormatDepth) {
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    } else {
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    }

    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;


    D3D_ASSERT(hr, GetD3D11Device()->CreateTexture2D (&desc, nullptr, &_texture));

    if (attachmentDescriptor.format == kRTFormatDepth) {
        D3D_ASSERT(hr, GetD3D11Device()->CreateDepthStencilView(_texture.Get(), nullptr, &_dsv));
    } else {
        D3D_ASSERT(hr, GetD3D11Device()->CreateRenderTargetView(_texture.Get(), nullptr, &_rtv));
        D3D_ASSERT(hr, GetD3D11Device()->CreateShaderResourceView(_texture.Get(), nullptr, &_srv));
    }

    return true;
}

Size RenderTarget::getSize() const {
    D3D11_TEXTURE2D_DESC desc;
    _texture->GetDesc(&desc);
    return { desc.Width, desc.Height };
}

UInt32 RenderTarget::getMSAASamples() const {
    D3D11_TEXTURE2D_DESC desc;
    _texture->GetDesc(&desc);
    return desc.SampleDesc.Count;
}

void RenderTarget::deinit() {
    _dsv.Reset();
    _srv.Reset();
    _rtv.Reset();
    _texture.Reset();
}


}