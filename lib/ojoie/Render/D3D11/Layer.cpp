//
// Created by aojoie on 5/23/2023.
//

#include "Render/private/D3D11/Device.hpp"
#include "Render/private/D3D11/Layer.hpp"

#include "Render/QualitySettings.hpp"
#include "Core/Configuration.hpp"

namespace AN {

namespace D3D11 {

[[noreturn]] AN_API void AbortOnD3D11Error();

}


bool AN::D3D11::Layer::init(AN::Window *window) {
    _window = (WIN::Window *)window;

    DXGI_SWAP_CHAIN_DESC1 sd{};

    Rect rect = window->getFrame();
    sd.Width = rect.width();
    sd.Height = rect.height();
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
#ifdef OJOIE_WITH_EDITOR
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; /// window with title bar using flip model will cause blank space !!!
#else
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif//OJOIE_WITH_EDITOR
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc{};
    fullscreenDesc.RefreshRate.Numerator = 0;
    fullscreenDesc.RefreshRate.Denominator = 1;
    fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    fullscreenDesc.Windowed = TRUE;

    /// look like using flip model will reduce frame rate when call SetWindowPos very often

    if (GetQualitySettings().getCurrent().vSyncCount == 0 &&
        (sd.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL || sd.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)) {
        sd.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }



    /// (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL and DXGI_SWAP_EFFECT_FLIP_DISCARD) do not support multisampling.
//    AntiAliasingMethod antiAliasingMethod = GetConfiguration().getObject<AntiAliasingMethod>("anti-aliasing");
//    UInt32             samples            = GetConfiguration().getObject<UInt32>("msaa-samples");
//
    HRESULT hr;
//    if (antiAliasingMethod == kAntiAliasingMSAA) {
//        UINT msaaQualityLevels;
//        D3D_ASSERT(hr, GetD3D11Device()->CheckMultisampleQualityLevels(
//                sd.BufferDesc.Format,
//                samples,                            // Sample count (4 is a common value)
//                &msaaQualityLevels            // Output number of quality levels
//        ));
//
//        if (msaaQualityLevels > 0) {
//            sd.SampleDesc.Count = samples;
//            sd.SampleDesc.Quality = msaaQualityLevels - 1;
//        }
//    }



#ifdef AN_DEBUG
    sd.Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    IDXGIFactory2 *factory2 = (IDXGIFactory2 *)GetDevice().getDXGIFactory();
    D3D_ASSERT(hr, factory2->CreateSwapChainForHwnd(GetDevice().getD3D11Device(),_window->getHWND(),
                                                  &sd, &fullscreenDesc, nullptr,
                                                  &_swapChain));

    DWORD dxgiFlags = DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES;
    GetDevice().getDXGIFactory()->MakeWindowAssociation(_window->getHWND(), dxgiFlags);

    if (FAILED(hr)) {
        AN_LOG(Error, "Fail to create d3d11 swapchain");
        return false;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    D3D_ASSERT(hr, _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer));

    ComPtr<ID3D11RenderTargetView> rtv;
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    D3D_ASSERT(hr, GetD3D11Device()->CreateRenderTargetView(backBuffer.Get(), &rtvDesc, &rtv));

    D3D11SetDebugName(rtv.Get(), "Swapchain-RTV");

    _rendertarget.bridgeSwapchain(backBuffer.Get(), rtv.Get());

    _presentable._renderTarget = AN::MakeObjectPtr<AN::RenderTarget>();

    ANAssert(_presentable._renderTarget->init()); // init empty renderTarget to bridge swapchain images

    _presentable._renderTarget->bridgeSwapchinRenderTargetInternal(&_rendertarget);
    _presentable._swapChain = _swapChain;

    return true;
}

D3D11::Layer::~Layer() {
    if (_swapChain) {
        _swapChain->SetFullscreenState(FALSE, nullptr);
    }
}

AN::Size AN::D3D11::Layer::getSize() {
    DXGI_SWAP_CHAIN_DESC desc;
    _swapChain->GetDesc(&desc);
    return { .width = desc.BufferDesc.Width, .height = desc.BufferDesc.Height };
}

void AN::D3D11::Layer::getDpiScale(float &x, float &y) {
    x = _window->getDPIScaleX();
    y = _window->getDPIScaleY();
}

AN::Window *AN::D3D11::Layer::getWindow() {
    return _window;
}

void D3D11::Layer::resize(const Size &size) {

    if (size == getSize()) {
        return;
    }

    DXGI_SWAP_CHAIN_DESC1 desc;
    _swapChain->GetDesc1(&desc);

    UINT swapCreateFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    if (GetQualitySettings().getCurrent().vSyncCount == 0 &&
        (desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL || desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD)) {
        swapCreateFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }


#ifdef AN_DEBUG
    swapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    _rendertarget.deinit();
    _presentable._renderTarget->bridgeSwapchinRenderTargetInternal(&_rendertarget); // remove ref


    D3D11::GetD3D11Context()->ClearState();
    D3D11::GetD3D11Context()->Flush();

    HRESULT hr;
    UInt32 width = std::max(8U, size.width);
    UInt32 height = std::max(8U, size.height);
    hr = _swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_UNKNOWN, swapCreateFlags);

    if (FAILED(hr)) {
        D3D11::AbortOnD3D11Error();
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    D3D_ASSERT(hr, _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer));

    ComPtr<ID3D11RenderTargetView> rtv;
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    D3D_ASSERT(hr, GetD3D11Device()->CreateRenderTargetView(backBuffer.Get(), &rtvDesc, &rtv));

    D3D11SetDebugName(rtv.Get(), "Swapchain-RTV");

    _rendertarget.bridgeSwapchain(backBuffer.Get(), rtv.Get());
    _presentable._renderTarget->bridgeSwapchinRenderTargetInternal(&_rendertarget);
}

void D3D11::Layer::setFullScreen(bool fullScreen) {
    _swapChain->SetFullscreenState(fullScreen, fullScreen ? GetDevice().getDXGIOutput() : nullptr);
}


}