//
// Created by aojoie on 5/22/2023.
//

#include "Render/private/D3D11/Device.hpp"
#include "Core/private/win32/App.hpp"
#include "Utility/Log.h"
#include "Utility/win32/Unicode.hpp"
#include <vector>

#include <d3d11_4.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DXGI.lib")

namespace AN::D3D11 {

typedef std::vector<D3D_FEATURE_LEVEL> SupportedFeatureLevels;

#include <dxgidebug.h>
#pragma comment(lib , "dxguid.lib")

static HANDLE gDeviceRemoveEvent = nullptr;
static DWORD gDeviceRemoveCookie = 0;
static PTP_WAIT gDeviceRemoveWait = nullptr;

#ifdef AN_DEBUG
static IDXGIInfoQueue* gD3D11InfoQueue;

static DXGI_INFO_QUEUE_MESSAGE_ID kIgnoreMessageIDs[] = {
    294, /// Directx11: IDXGIFactory::CreateSwapChain: Blt-model swap effects (DXGI_SWAP_EFFECT_DISCARD and DXGI_SWAP_EFFECT_SEQUENTIAL) are legacy swap effects
    355,  /// overlap vertex input
    356 /// Vertex Buffer at the input vertex slot 0 is not big enough for what the Draw*() call expects to traverse. This is OK, as reading off the end of the Buffer is defined to return 0. However the developer probably did not intend to make use of this behavior
};

__declspec(dllexport) void LogD3D11DebugMessage() {
    UINT64 message_count = gD3D11InfoQueue->GetNumStoredMessages(DXGI_DEBUG_ALL);

    for (UINT64 i = 0; i < message_count; i++){
        HRESULT hr;
        SIZE_T len;

        hr = gD3D11InfoQueue->GetMessage(DXGI_DEBUG_ALL,i, nullptr, &len);
        ANAssert(SUCCEEDED(hr));
        auto bytes = std::make_unique<byte[]>(len);
        auto message = reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(bytes.get());
        hr = gD3D11InfoQueue->GetMessage(DXGI_DEBUG_ALL, i, message, &len);

        ANAssert(SUCCEEDED(hr));

        bool ignore = false;
        for (DXGI_INFO_QUEUE_MESSAGE_ID id : kIgnoreMessageIDs) {
            if (message->ID == id) {
                ignore = true;
                break;
            }
        }

        /// ignore INFO
        if (message->Severity == DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO) {
            ignore = true;
        }

        if (ignore) continue;

        //do whatever you want to do with it
        AN_LOG(Debug, "Directx11: %.*s", (int)message->DescriptionByteLength, message->pDescription);
    }

    gD3D11InfoQueue->ClearStoredMessages(DXGI_DEBUG_ALL);
}
#endif

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

__declspec(dllexport) void D3D11SetDebugName(ID3D11DeviceChild* obj, const std::string_view &name) {
    if (obj) {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.data());
    }
}

SupportedFeatureLevels GetSupportedFeatureLevels() {
    SupportedFeatureLevels features;

    features.push_back(D3D_FEATURE_LEVEL_11_0);
    features.push_back(D3D_FEATURE_LEVEL_10_1);
    features.push_back(D3D_FEATURE_LEVEL_10_0);

    return features;
}

static void SelectOutput(IDXGIAdapter *adapter, int outputIndex, IDXGIOutput **ppOutput) {
    if (outputIndex == 0)
        *ppOutput = nullptr;

    ANAssert(adapter);

    IDXGIOutput *output = NULL;
    for (int i = 0; SUCCEEDED(adapter->EnumOutputs(i, &output)); ++i) {
        if (i == outputIndex)
            break;
        else
            output->Release();
    }
    *ppOutput = output;
}

static void SelectAdapter(int adapterIndex, ComPtr<IDXGIFactory> &outFactory, ComPtr<IDXGIAdapter> &adapter)
{
    if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&outFactory)) )
    {
        ComPtr<IDXGIFactory6> factory6;

        if (SUCCEEDED(outFactory.As(&factory6))) {

            for (int i = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(i,
                                                                           DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                                           IID_PPV_ARGS(&adapter))); ++i) {
                if (i == adapterIndex)
                    break;
            }

        } else {
            for (int i = 0; SUCCEEDED(outFactory->EnumAdapters(i, &adapter)); ++i) {
                if (i == adapterIndex)
                    break;
            }
        }
    }
}

[[noreturn]] __declspec(dllexport) void AbortOnD3D11Error() {
    HRESULT hr = GetD3D11Device()->GetDeviceRemovedReason();

    std::string message;

    if (hr == S_OK) {
        hr = GetLastError();
        message = std::format("D3D11 Error\n{}", WIN::TranslateErrorCode(GetLastError()));
    } else {
        message = "D3D11 Device Reset/Remove";
    }

    AN_LOG(Error, "%s", message.c_str());
    MessageBoxW(nullptr, Utf8ToWide(message).c_str(),
                L"Ojoie Error", MB_ICONERROR | MB_OK);
    exit(-1);
}

VOID CALLBACK D3D11DeviceLostWaitCallback(
        _Inout_     PTP_CALLBACK_INSTANCE Instance,
        _Inout_opt_ PVOID                 Context,
        _Inout_     PTP_WAIT              Wait,
        _In_        TP_WAIT_RESULT        WaitResult
) {
    AbortOnD3D11Error();
}

Device::Device() : _outputIndex() {}

bool Device::init() {
    DWORD d3d11CreateFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef AN_DEBUG
    d3d11CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    SupportedFeatureLevels features = GetSupportedFeatureLevels();
    D3D_FEATURE_LEVEL      level;

    SelectAdapter(0, _DXGIFactory, _DXGIAdapter);
    HRESULT hr = D3D11CreateDevice(_DXGIAdapter.Get(),
                                   D3D_DRIVER_TYPE_UNKNOWN,
                                   nullptr,
                                   d3d11CreateFlags,
                                   features.data(),
                                   features.size(),
                                   D3D11_SDK_VERSION,
                                   &_d3d11Device,
                                   &level,
                                   &_context);

    if (FAILED(hr)) {
        AN_LOG(Error, "Fail to create D3D11 Device");
        return false;
    }

    AN_LOG(Info, "D3D11Context 0x%p Created", _context.Get());


    ComPtr<IDXGIDevice> dxgiDevice = nullptr;
    hr = _d3d11Device->QueryInterface(__uuidof(IDXGIDevice), &dxgiDevice);
    ANAssert(SUCCEEDED(hr));

    if (_DXGIAdapter == nullptr) {
        ComPtr<IDXGIAdapter> dxgiAdapter = nullptr;

        hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), &_DXGIAdapter);
        ANAssert(SUCCEEDED(hr));

        hr = _DXGIAdapter->GetParent(__uuidof(IDXGIFactory2), &_DXGIFactory);
        ANAssert(SUCCEEDED(hr));

        SelectOutput(_DXGIAdapter.Get(), _outputIndex, &_output);
    }


    ComPtr<ID3D11Device4> d3d11Device4;
    if (SUCCEEDED(_d3d11Device->QueryInterface(__uuidof(ID3D11Device4), &d3d11Device4))) {
        gDeviceRemoveEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        d3d11Device4->RegisterDeviceRemovedEvent(gDeviceRemoveEvent, &gDeviceRemoveCookie);
        gDeviceRemoveWait = CreateThreadpoolWait(D3D11DeviceLostWaitCallback, nullptr, nullptr);
        SetThreadpoolWait(gDeviceRemoveWait, gDeviceRemoveEvent, nullptr);
    }

#ifdef AN_DEBUG
    typedef HRESULT (WINAPI* DXGIGETDebugInterface)(REFIID, void**);

    HMODULE hModDxgiDebug = LoadLibraryEx(TEXT("dxgidebug.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    ANAssert(hModDxgiDebug != nullptr);

    const auto dxgiGetDebugInterface = reinterpret_cast<DXGIGETDebugInterface>(
            reinterpret_cast<void*>(GetProcAddress(hModDxgiDebug, "DXGIGetDebugInterface"))
    );

    ANAssert(dxgiGetDebugInterface != nullptr);

    hr = dxgiGetDebugInterface(__uuidof(IDXGIInfoQueue),(void **)&gD3D11InfoQueue);
    ANAssert(SUCCEEDED(hr));
#endif

    hr = _context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), &_annotation);
    ANAssert(SUCCEEDED(hr));

    return true;
}

void Device::deinit() {

    _context->ClearState();

    if (gDeviceRemoveEvent) {
        CloseThreadpoolWait(gDeviceRemoveWait);
        ComPtr<ID3D11Device4> d3d11Device4;
        if (SUCCEEDED(_d3d11Device->QueryInterface(__uuidof(ID3D11Device4), &d3d11Device4))) {
            d3d11Device4->UnregisterDeviceRemoved(gDeviceRemoveCookie);
        }

        CloseHandle(gDeviceRemoveEvent);
        gDeviceRemoveEvent = nullptr;
    }

    AN_LOG(Info, "D3D11Context 0x%p Destroyed", _context.Get());
    _output.Reset();
    _annotation.Reset();
    _DXGIAdapter.Reset();
    _DXGIFactory.Reset();
    _context.Reset();
    _d3d11Device.Reset();
}

static Device gDevice;

bool InitializeDevice() {
    return gDevice.init();
}
void DestroyDevice() {
#ifdef AN_DEBUG
    ID3D11Debug *d3dDebug;
    HRESULT hr = gDevice.getD3D11Device()->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3dDebug));
    if (SUCCEEDED(hr))
    {
        d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
    }
    if (d3dDebug != nullptr)
        d3dDebug->Release();

    gD3D11InfoQueue->Release();
#endif
    gDevice.deinit();
}
Device &GetDevice() {
    return gDevice;
}

}// namespace AN::D3D11