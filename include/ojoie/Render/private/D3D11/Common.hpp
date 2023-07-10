//
// Created by aojoie on 5/23/2023.
//

#pragma once

#include <wrl/client.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <ojoie/Render/private/D3D11/RenderTypes.hpp>
#include <ojoie/Render/private/D3D11/Device.hpp>
#include <ojoie/Render/RenderContext.hpp>

#include <format>

namespace AN::D3D11 {

using Microsoft::WRL::ComPtr;

#ifdef AN_DEBUG

void LogD3D11DebugMessage();

#ifdef __cpp_lib_source_location
#define D3D_ASSERT(hr, call) \
    do { hr = call;          \
        if (FAILED(hr)) {    \
            LogD3D11DebugMessage(); \
            __an_assert_log(#call); \
            __an_assert_fail();\
        }                    \
    } while (0)
#else
#define D3D_ASSERT(hr, call) \
    do { hr = call;          \
        if (FAILED(hr)) {    \
            LogD3D11DebugMessage(); \
            __an_assert_log(#call, __FILE__, __LINE__, __func__); \
            __an_assert_fail();\
        }                    \
    } while (0)
#endif//__cpp_lib_source_location

#else
#define D3D_ASSERT(hr, call) hr = call
#endif

void D3D11SetDebugName(ID3D11DeviceChild* obj, const std::string_view &name);

}