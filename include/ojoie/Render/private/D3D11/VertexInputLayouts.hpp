//
// Created by aojoie on 5/24/2023.
//

#pragma once
#include <ojoie/Render/VertexBuffer.hpp>
#include <ojoie/Render/private/D3D11/Common.hpp>

#include <vector>
#include <set>
#include <map>

namespace AN::D3D11 {

struct InputSignatureD3D11 {
    std::vector<UInt8> blob;
    bool operator < (const InputSignatureD3D11& o) const {
        size_t sizeA = blob.size();
        size_t sizeB = o.blob.size();
        if (sizeA != sizeB)
            return sizeA < sizeB;
        int res = memcmp (blob.data(), o.blob.data(), sizeA);
        return res < 0;
    }
};

class VertexInputLayouts {

    struct KeyType {
        bool operator < (const KeyType& rhs) const;
        ChannelInfoArray channels;
        const InputSignatureD3D11* inputSig;
        UInt32 extraBits;
    };

    typedef std::set<InputSignatureD3D11> InputSignatures;
    InputSignatures	m_InputSignatures;

    typedef std::map<KeyType, ComPtr<ID3D11InputLayout>> VertexDeclMap;
    VertexDeclMap m_VertexDeclMap;

    typedef std::map<const InputSignatureD3D11*, ComPtr<ID3D11InputLayout>> ImmVertexDeclMap;
    ImmVertexDeclMap m_ImmVertexDeclMap;

public:

    ID3D11InputLayout* getVertexInputLayout(const ChannelInfoArray& channels, const InputSignatureD3D11* inputSig);
    ID3D11InputLayout* getImmVertexDecl(const InputSignatureD3D11* inputSig);

    const InputSignatureD3D11* getShaderInputSignature(void* code, unsigned length);

    void cleanup();

};

VertexInputLayouts &GetVertexInputLayouts();

}
