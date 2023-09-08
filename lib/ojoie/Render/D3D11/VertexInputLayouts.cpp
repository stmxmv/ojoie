//
// Created by aojoie on 5/24/2023.
//

#include "Render/private/D3D11/VertexInputLayouts.hpp"
#include "Render/private/D3D11/Device.hpp"

void D3DHash(const unsigned char *data, unsigned size, unsigned char res[16]);

namespace AN::D3D11 {

#define MAKE_FOURCC(a, b, c, d) ((UInt32) (UInt8) (a) | ((UInt32) (UInt8) (b) << 8) | ((UInt32) (UInt8) (c) << 16) | ((UInt32) (UInt8) (d) << 24))
#define kFOURCC_DXBC            MAKE_FOURCC('D', 'X', 'B', 'C')

// Shader input signature
#define kFOURCC_ISGN MAKE_FOURCC('I', 'S', 'G', 'N')

struct DXBCChunkHeader {
    unsigned fourcc;
    unsigned size;
};

struct DXBCHeader {
    UInt32 fourcc;
    UInt32 hash[4];
    UInt32 one;
    UInt32 total_size;
    UInt32 chunk_count;
};

DXBCChunkHeader *dxbc_find_chunk(const void *data, int size, unsigned fourcc) {
    DXBCHeader *header        = (DXBCHeader *) data;
    UInt32     *chunk_offsets = (UInt32 *) (header + 1);
    if (header->fourcc != kFOURCC_DXBC)
        return 0;
    unsigned num_chunks = header->chunk_count;
    for (unsigned i = 0; i < num_chunks; ++i) {
        unsigned         offset = chunk_offsets[i];
        DXBCChunkHeader *chunk  = (DXBCChunkHeader *) ((char *) data + offset);
        if (chunk->fourcc == fourcc)
            return chunk;
    }
    return 0;
}

static void dxbc_create_internal(struct DXBCChunkHeader **chunks, unsigned num_chunks, void *buffer, unsigned total_size) {
    DXBCHeader *header = (DXBCHeader *) buffer;

    header->fourcc = kFOURCC_DXBC;
    memset(header->hash, 0, sizeof(header->hash));

    header->one         = 1;
    header->total_size  = total_size;
    header->chunk_count = num_chunks;

    UInt32 *chunk_offsets = (UInt32 *) (header + 1);
    UInt32  off           = sizeof(struct DXBCHeader) + num_chunks * sizeof(UInt32);
    for (unsigned i = 0; i < num_chunks; ++i) {
        chunk_offsets[i]         = off;
        unsigned chunk_full_size = sizeof(DXBCChunkHeader) + chunks[i]->size;
        memcpy((char *) header + off, chunks[i], chunk_full_size);
        off += chunk_full_size;
    }

    D3DHash((const UInt8 *) &header->one, total_size - 20, (UInt8 *) header->hash);
}

void dxbc_create(struct DXBCChunkHeader **chunks, unsigned num_chunks, std::vector<UInt8> &out) {
    size_t data_size = 0;
    for (unsigned i = 0; i < num_chunks; ++i)
        data_size += sizeof(UInt32) + sizeof(DXBCChunkHeader) + chunks[i]->size;
    const size_t total_size = sizeof(DXBCHeader) + data_size;

    out.resize(total_size);

    dxbc_create_internal(chunks, num_chunks, out.data(), total_size);
}

static D3D11_INPUT_ELEMENT_DESC kChannelVertexElems[kShaderChannelCount] = {
    // semantic name, semantic index, format, input slot, aligned byte offset, input slot class, instance data step rate
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,       0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static const D3D11_INPUT_ELEMENT_DESC kImmChannelVertexElems[] = {
    // semantic name, semantic index, format, input slot, aligned byte offset, input slot class, instance data step rate
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 52, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 3, DXGI_FORMAT_R32G32B32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 4, DXGI_FORMAT_R32G32B32_FLOAT, 0, 76, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT, 0, 88, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 6, DXGI_FORMAT_R32G32B32_FLOAT, 0,100, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 7, DXGI_FORMAT_R32G32B32_FLOAT, 0,112, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    // feed position as tangent0 data, just in case we use shaders that pretend to want tangents
    { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static DXGI_FORMAT GetD3D11VertexDeclType(const ChannelInfo &info) {
    switch (info.format) {
        case kChannelFormatFloat: {
            switch (info.dimension) {
                case 1:
                    return DXGI_FORMAT_R32_FLOAT;
                case 2:
                    return DXGI_FORMAT_R32G32_FLOAT;
                case 3:
                    return DXGI_FORMAT_R32G32B32_FLOAT;
                case 4:
                    return DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
            break;
        }
        case kChannelFormatFloat16: {
            switch (info.dimension) {
                case 2:
                    return DXGI_FORMAT_R16G16_FLOAT;
                case 4:
                    return DXGI_FORMAT_R16G16B16A16_FLOAT;
            }
            break;
        }
        case kChannelFormatColor: {
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        }
    }
    ANAssert("No matching D3D11 vertex decl type!");
    return DXGI_FORMAT_UNKNOWN;
}

bool VertexInputLayouts::KeyType::operator< (const VertexInputLayouts::KeyType &rhs) const {
    if (inputSig != rhs.inputSig)
        return inputSig < rhs.inputSig;
    if (extraBits != rhs.extraBits)
        return extraBits < rhs.extraBits;
    return memcmp(channels, rhs.channels, sizeof(channels)) < 0;
}

ID3D11InputLayout *VertexInputLayouts::getVertexInputLayout(const ChannelInfoArray& channels,
                                                            const InputSignatureD3D11 *inputSig) {

    if (!inputSig) {
        ANAssert("DX11 shader input signature is nullptr");
        return nullptr;
    }

    KeyType key;
    memcpy(key.channels, channels, sizeof(key.channels));
    key.extraBits =  0;
    key.inputSig = inputSig;

    VertexDeclMap::iterator it = m_VertexDeclMap.find (key);
    if( it != m_VertexDeclMap.end() )
        return it->second.Get();

    D3D11_INPUT_ELEMENT_DESC elements[kShaderChannelCount];

    int elIndex = 0;
    for( int chan = 0; chan < kShaderChannelCount; chan++ ) {
        ANAssert(elIndex < kShaderChannelCount);
        if (!channels[chan].isValid() ) {
            /// TODO for now, hack in all shader channels to pretend to be there
            elements[elIndex] = kChannelVertexElems[chan];
            elements[elIndex].AlignedByteOffset = 0;
            ++elIndex;
            continue;
        }
        elements[elIndex] = kChannelVertexElems[chan];
        elements[elIndex].InputSlot = channels[chan].stream;
        elements[elIndex].AlignedByteOffset = channels[chan].offset;
        elements[elIndex].Format = GetD3D11VertexDeclType(channels[chan]);
        ++elIndex;
    }

    ComPtr<ID3D11InputLayout> decl;
    HRESULT hr;
    D3D_ASSERT(hr, GetD3D11Device()->CreateInputLayout(elements,
                                                       elIndex,
                                                       inputSig->blob.data(),
                                                       inputSig->blob.size(),
                                                       &decl));

    D3D11SetDebugName(decl.Get(), std::format("InputLayout-{}", elIndex));
    m_VertexDeclMap.insert( std::make_pair( key, decl ) );

    return decl.Get();
}

ID3D11InputLayout *VertexInputLayouts::getImmVertexDecl(const InputSignatureD3D11 *inputSig) {
    if (!inputSig) {
        ANAssert("DX11 shader input signature is nullptr");
        return nullptr;
    }

    // already have vertex declaration for this shader?
    ImmVertexDeclMap::iterator it = m_ImmVertexDeclMap.find (inputSig);
    if (it != m_ImmVertexDeclMap.end())
        return it->second.Get();

    ComPtr<ID3D11InputLayout> decl = nullptr;
    HRESULT hr;
    D3D_ASSERT(hr,GetD3D11Device()->CreateInputLayout (kImmChannelVertexElems, std::size(kImmChannelVertexElems),
                                                       inputSig->blob.data(), inputSig->blob.size(), &decl));

    D3D11SetDebugName(decl.Get(), "InputLayoutImmediate");
    m_ImmVertexDeclMap.insert(std::make_pair(inputSig, decl));

    return decl.Get();
}


const InputSignatureD3D11 *VertexInputLayouts::getShaderInputSignature(void *code, unsigned int length) {
    DXBCChunkHeader *isigChunk = dxbc_find_chunk(code, length, kFOURCC_ISGN);
    ANAssert(isigChunk);
    if (!isigChunk)
        return nullptr;

    InputSignatureD3D11 isig;
    dxbc_create(&isigChunk, 1, isig.blob);

    InputSignatures::iterator it = m_InputSignatures.insert(isig).first;
    return &(*it);
}



void VertexInputLayouts::cleanup() {
    m_VertexDeclMap.clear();
    m_ImmVertexDeclMap.clear();
    m_InputSignatures.clear();
}

VertexInputLayouts &GetVertexInputLayouts() {
    static VertexInputLayouts vertexInputLayouts;
    return vertexInputLayouts;
}

}// namespace AN::D3D11