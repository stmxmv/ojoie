//
// Created by aojoie on 5/22/2023.
//

#pragma once

#include <ojoie/Export/Export.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3d11_1.h>

namespace AN::D3D11 {

using Microsoft::WRL::ComPtr;

class AN_API Device {

    int _outputIndex;
    ComPtr<ID3D11Device> _d3d11Device;
    ComPtr<ID3D11DeviceContext> _context;
    ComPtr<IDXGIFactory> _DXGIFactory;
    ComPtr<ID3DUserDefinedAnnotation> _annotation;
    ComPtr<IDXGIOutput> _output;
    ComPtr<IDXGIAdapter> _DXGIAdapter;

public:

    Device();

    bool init();

    void deinit();

    ID3D11Device *getD3D11Device() const { return _d3d11Device.Get();}

    IDXGIFactory *getDXGIFactory() const { return _DXGIFactory.Get(); }

    ID3D11DeviceContext *getD3D11Context() const { return _context.Get(); }

    ID3DUserDefinedAnnotation *getAnnotation() const { return _annotation.Get(); }

    IDXGIOutput *getDXGIOutput() const { return _output.Get(); }
};

AN_API bool InitializeDevice();
AN_API void DestroyDevice();

AN_API Device &GetDevice();

inline ID3D11Device *GetD3D11Device() {
    return GetDevice().getD3D11Device();
}

inline ID3D11DeviceContext *GetD3D11Context() {
    return GetDevice().getD3D11Context();
}

inline ID3DUserDefinedAnnotation *GetD3D11Annotation() {
    return GetDevice().getAnnotation();
}

}
