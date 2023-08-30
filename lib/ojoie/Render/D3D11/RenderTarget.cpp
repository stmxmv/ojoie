//
// Created by aojoie on 5/23/2023.
//

#include "Render/private/D3D11/Rendertarget.hpp"
#include "Render/private/D3D11/Device.hpp"



namespace AN::D3D11 {

bool RenderTarget::init(const RenderTargetDescriptor &attachmentDescriptor) {

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

    if (attachmentDescriptor.format == kRTFormatDepth || attachmentDescriptor.format == kRTFormatShadowMap) {
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    } else {
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    }

    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;


    D3D_ASSERT(hr, GetD3D11Device()->CreateTexture2D (&desc, nullptr, &_texture));

    D3D11SetDebugName(_texture.Get(), std::format("RenderTarget-{}x{}", desc.Width, desc.Height));

//    if (desc.Format == DXGI_FORMAT_R8G8B8A8_TYPELESS) {
//        /// default use sRGB
//        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
//    }

    if (attachmentDescriptor.format == kRTFormatDepth || attachmentDescriptor.format == kRTFormatShadowMap) {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = desc.Format;
        if (desc.SampleDesc.Count > 1) {
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        } else {
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        }
        if (desc.Format == DXGI_FORMAT_R16_TYPELESS) {
            dsvDesc.Format = DXGI_FORMAT_D16_UNORM;
        }
        D3D_ASSERT(hr, GetD3D11Device()->CreateDepthStencilView(_texture.Get(), &dsvDesc, &_dsv));

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = desc.Format;
        if (desc.SampleDesc.Count > 1) {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        } else {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        }
        srvDesc.Texture2D.MipLevels = 1;
        if (desc.Format == DXGI_FORMAT_R16_TYPELESS) {
            srvDesc.Format = DXGI_FORMAT_R16_UNORM;
        }
        D3D_ASSERT(hr, GetD3D11Device()->CreateShaderResourceView(_texture.Get(), &srvDesc, &_srv));

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