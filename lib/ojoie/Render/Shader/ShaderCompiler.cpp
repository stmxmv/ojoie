//
// Created by aojoie on 4/13/2023.
//

#include "Render/Shader/ShaderCompiler.hpp"
#include "Allocator/MemoryDefines.h"
#include "Render/RenderContext.hpp"
#include "Render/private/D3D11/Device.hpp"
#include "Utility/win32/Unicode.hpp"
#include <ojoie/Render/private/D3D11/Common.hpp>
#include <ojoie/Utility/Log.h>

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <minwindef.h>
#include <Unknwnbase.h>

#ifdef OJOIE_USE_VULKAN
#include <inc/dxcapi.h>
#endif

#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <unordered_set>

using Microsoft::WRL::ComPtr;

namespace AN::RC {

#ifdef OJOIE_USE_VULKAN
static ComPtr<IDxcLibrary>        gLibrary;
static ComPtr<IDxcCompiler3>      gCompiler;
static ComPtr<IDxcUtils>          gUtils;
static ComPtr<IDxcIncludeHandler> gDefaultIncludeHandler;


class ANIncludeHandler : public IDxcIncludeHandler {
public:
    HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR                             pFilename,
                                         _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource) override {
        ComPtr<IDxcBlobEncoding> pEncoding;
        std::filesystem::path    path(pFilename);
        std::filesystem::path    canonicalPath = std::filesystem::weakly_canonical(path);
        std::string              npath         = canonicalPath.make_preferred().string();
        if (IncludedFiles.find(npath) != IncludedFiles.end()) {
            // Return empty string blob if this file has been included before
            static const char nullStr[] = " ";
            gUtils->CreateBlob(nullStr, ARRAYSIZE(nullStr), CP_UTF8, pEncoding.GetAddressOf());
            *ppIncludeSource = pEncoding.Detach();
            return S_OK;
        }

        HRESULT hr = gUtils->LoadFile(pFilename, nullptr, pEncoding.GetAddressOf());
        if (SUCCEEDED(hr)) {
            IncludedFiles.insert(npath);
            *ppIncludeSource = pEncoding.Detach();
        } else {
            *ppIncludeSource = nullptr;
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override {
        return gDefaultIncludeHandler->QueryInterface(riid, ppvObject);
    }

    ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
    ULONG STDMETHODCALLTYPE Release(void) override { return 0; }

    std::unordered_set<std::string> IncludedFiles;
};
#endif //OJOIE_USE_VULKAN

class D3DInclude : public ID3DInclude {

    std::vector<std::string> _includePaths;

public:

    void addPath(std::string_view path) {
        _includePaths.emplace_back(path);
    }

    STDMETHOD(Open)(D3D_INCLUDE_TYPE IncludeType,
                    LPCSTR pFileName, LPCVOID pParentData,
                    LPCVOID* ppData, UINT* pBytes) override {

        for (auto &path : _includePaths) {
            std::filesystem::path filePath(path);
            filePath.append(pFileName);
            if (exists(filePath)) {
                std::ifstream file(filePath.string(), std::ios::binary);
                file.seekg(0, std::ios::end);
                uint64_t read_count = static_cast<uint64_t>(file.tellg());
                file.seekg(0, std::ios::beg);

                void *data = AN_MALLOC(read_count);
                file.read(reinterpret_cast<char *>(data), (std::streamsize)read_count);
                file.close();

                *ppData = data;
                *pBytes = read_count;

                return S_OK;
            }
        }
        *ppData = nullptr;
        *pBytes = 0;

        return E_FAIL;
    }

    STDMETHOD(Close)(LPCVOID pData) override {
        AN_FREE((void *)pData);
        return S_OK;
    }
};

ShaderCompiler::ShaderCompiler() {

    if (GetGraphicsAPI() == kGraphicsAPIVulkan) {
#ifdef OJOIE_USE_VULKAN
        HRESULT hres;
        if (gLibrary == nullptr) {
            hres = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&gLibrary));
            if (FAILED(hres)) {
                AN_LOG(Error, "Could not init DXC Library");
                return;
            }
        }

        if (gCompiler == nullptr) {
            hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&gCompiler));
            if (FAILED(hres)) {
                AN_LOG(Error, "Could not init DXC Compiler");
                return;
            }
        }

        if (gUtils == nullptr) {
            hres = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&gUtils));
            if (FAILED(hres)) {
                AN_LOG(Error, "Could not init DXC Utiliy");
                return;
            }
        }

        if (gDefaultIncludeHandler == nullptr) {
            hres = gUtils->CreateDefaultIncludeHandler(&gDefaultIncludeHandler);
            if (FAILED(hres)) {
                AN_LOG(Error, "Could not init default include handler");
                return;
            }
        }
#endif //OJOIE_USE_VULKAN
    }

}


std::vector<UInt8> ShaderCompiler::compileHLSLToSPIRV(ShaderStage        stage,
                                                      const char             *source,
                                                      const char             *entry,
                                                      const char             *nameHint,
                                                      std::span<const char *> include,
                                                      bool emitSemantic) {
#ifdef OJOIE_USE_VULKAN
    LPCWSTR targetProfile;
    switch (stage) {
        case kShaderStageVertex:
            targetProfile = L"vs_6_6";
            break;
        case kShaderStageFragment:
            targetProfile = L"ps_6_6";
            break;
        default:
            AN_LOG(Error, "Not support shader stage %d", stage);
            return {};
    }

    std::wstring   wEntry;
    wEntry.resize(MultiByteToWideChar(CP_UTF8, 0, entry, (int)strlen(entry) + 1, nullptr, 0));
    MultiByteToWideChar(CP_UTF8, 0, entry, (int)strlen(entry) + 1, wEntry.data(), (int)wEntry.size());
    std::wstring   wNameHint;
    const wchar_t *wNameHint_c = L"ANBinaryShaderFile";
    if (nameHint) {
        wNameHint.assign(nameHint, nameHint + strlen(nameHint));
        wNameHint_c = wNameHint.c_str();
    }

    std::vector<LPCWSTR> arguments = {
            // (Optional) name of the shader file to be displayed e.g. in an error message
            wNameHint_c,
            // Shader main entry point
            L"-E",
            wEntry.c_str(),
            // Shader target profile
            L"-T",
            targetProfile,
            // set auto bind to descriptor set 1
            L"-auto-binding-space",
            L"1",
            // Compile to SPIRV
            L"-spirv",
    };

    if (emitSemantic) {
        // add HLSL semantic
        arguments.push_back(L"-fspv-reflect");
    }

    std::vector<std::wstring> wIncludes;
    wIncludes.reserve(include.size());
    for (const char *i : include) {
        std::wstring wInclude;
        wInclude.resize(MultiByteToWideChar(CP_UTF8, 0, i, (int)strlen(i) + 1, nullptr, 0));
        MultiByteToWideChar(CP_UTF8, 0, i, (int)strlen(i) + 1, wInclude.data(), (int)wInclude.size());

        wIncludes.push_back(wInclude);
        arguments.push_back(L"-I");
        arguments.push_back(wIncludes.back().c_str());
    }

    DxcBuffer buffer{};
    buffer.Encoding = DXC_CP_UTF8;
    buffer.Ptr      = source;
    buffer.Size     = strlen(source);


    ComPtr<IDxcResult> result{};
    HRESULT            hres = gCompiler->Compile(
            &buffer,
            arguments.data(),
            (uint32_t) std::size(arguments),
            gDefaultIncludeHandler.Get(),
            IID_PPV_ARGS(&result));

    if (SUCCEEDED(hres)) {
        result->GetStatus(&hres);
    }

    if (FAILED(hres) && (result)) {
        ComPtr<IDxcBlobEncoding> errorBlob;
        hres = result->GetErrorBuffer(&errorBlob);
        if (SUCCEEDED(hres) && errorBlob) {
            AN_LOG(Warning, "Shader compilation failed :%s", (const char *) errorBlob->GetBufferPointer());
            return {};
        }
    }

    ComPtr<IDxcBlob> code;
    result->GetResult(&code);

    std::vector<UInt8> ret;
    ret.resize(code->GetBufferSize());
    memcpy(ret.data(), code->GetBufferPointer(), code->GetBufferSize());
    return ret;
#else
    return {};
#endif //OJOIE_USE_VULKAN
}

std::vector<UInt8> ShaderCompiler::compileHLSLToCSO(ShaderStage stage,
                                                    const char *source,
                                                    const char *entry,
                                                    const char *nameHint,
                                                    std::span<const char *> include,
                                                    bool emitSemantic) {

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if AN_DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif

    LPCSTR targetProfile;
    switch (stage) {
        case kShaderStageVertex:
            targetProfile = "vs_5_0";
            break;
        case kShaderStageFragment:
            targetProfile = "ps_5_0";
            break;
        default:
            AN_LOG(Error, "Not support shader stage %d", stage);
            return {};
    }

    ComPtr<ID3DBlob> shaderBlob = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    D3DInclude d3dInclude;

    for (const char *inc : include) {
        d3dInclude.addPath(inc);
    }

    HRESULT hr = D3DCompile(source, strlen(source), nameHint, nullptr, &d3dInclude,
                            entry, targetProfile,
                            flags, 0, &shaderBlob, &errorBlob);

    if (FAILED(hr)) {
        AN_LOG(Error, "Shader compilation failed :%s", (char *) errorBlob->GetBufferPointer());
        return {};
    }


    std::vector<UInt8> ret;
    ret.resize(shaderBlob->GetBufferSize());
    memcpy(ret.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
    return ret;
}


}// namespace AN::RC