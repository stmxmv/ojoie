//
// Created by aojoie on 5/24/2023.
//

#include "Render/private/D3D11/UniformBuffers.hpp"
#include "Render/private/D3D11/Device.hpp"
#include "Render/private/D3D11/CommandBuffer.hpp"

namespace AN::D3D11 {


void UniformBuffers::destroyAll() {
    m_BufferKeys.clear();
    m_Buffers.clear();
}


void UniformBuffers::resetBinds() {
    for (ConstBuffers::iterator it = m_Buffers.begin(), itEnd = m_Buffers.end(); it != itEnd; ++it) {
        for (int i = 0; i < kShaderStageCount; ++i) {
            it->bindIndex[i] = -1;
        }
        it->bindStages = 0;
    }
}

void UniformBuffers::reset() {
    memset(m_ActiveBuffers, 0, sizeof(m_ActiveBuffers));
}

void UniformBuffers::setBufferInfo(int id, int size) {
    size_t n   = m_Buffers.size();
    UInt32 key = id | (size << 16);
    for (size_t i = 0; i < n; ++i) {
        if (m_BufferKeys[i] == key)
            return;
    }

    ConstBuffer cb;
    cb.data.resize(size);
    cb.dirty = true;
    for (int i = 0; i < kShaderStageCount; ++i)
        cb.bindIndex[i] = -1;
    cb.bindStages = 0;

    ID3D11Device *dev = GetD3D11Device();
    // Default usage and using UpdateSubresource is seemingly preferred path in drivers
    // over dynamic buffer with Map.
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth           = size;
    desc.Usage               = D3D11_USAGE_DEFAULT;
    desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags      = 0;
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;

    HRESULT hr;
    D3D_ASSERT(hr, dev->CreateBuffer(&desc, NULL, &cb.buffer));
    D3D11SetDebugName(cb.buffer.Get(), std::format("ConstantBuffer-{}-{}", id, size));
    m_Buffers.push_back(cb);
    m_BufferKeys.push_back(key);
}

int UniformBuffers::findAndBind(int id, ShaderStage shaderType, int bind, int size) {
    UInt32 key = id | (size << 16);
    int    idx = 0;
    for (ConstBufferKeys::const_iterator it = m_BufferKeys.begin(), itEnd = m_BufferKeys.end(); it != itEnd; ++it, ++idx) {
        if (*it == key) {
            ConstBuffer &cb = m_Buffers[idx];
            if (bind >= 0) {
                cb.bindIndex[shaderType] = bind;
                cb.bindStages |= (1<<shaderType);
            }
            return idx;
        }
    }
    ANAssert(false);
    return -1;
}

void UniformBuffers::setConstant(int idx, int offset, const void *data, int size) {
    ANAssert(idx >= 0 && idx < m_Buffers.size());
    ConstBuffer &cb = m_Buffers[idx];
    ANAssert(offset >= 0 && offset + size <= (m_BufferKeys[idx] >> 16) && size > 0);


    if (size == 4) {
        UInt32 *dstData = (UInt32 *) (cb.data.data() + offset);
        UInt32  srcData = *(UInt32 *) data;
        if (*dstData != srcData) {
            *dstData = srcData;
            cb.dirty = true;
        }
    } else {
        if (memcmp(cb.data.data() + offset, data, size) != 0) {
            memcpy(cb.data.data() + offset, data, size);
            cb.dirty = true;
        }
    }
}

void UniformBuffers::updateBuffers(AN::CommandBuffer *_commandBuffer) {
    D3D11::CommandBuffer *commandBuffer = (D3D11::CommandBuffer *)_commandBuffer;
    ID3D11DeviceContext *ctx = commandBuffer->getContext();

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr;
    size_t n = m_Buffers.size();

    for (size_t i = 0; i < n; ++i) {
        ConstBuffer& cb = m_Buffers[i];
        if (cb.bindStages == 0)
            continue;
        if (cb.dirty) {
            ctx->UpdateSubresource(cb.buffer.Get(), 0, NULL, cb.data.data(), (m_BufferKeys[i]>>16), 1);
        }

        int bindIndex;

        // Bind to used stages
#define BIND_CB(cbShaderType, dxCall) \
		bindIndex = cb.bindIndex[cbShaderType]; \
		if (bindIndex >= 0 && (m_ActiveBuffers[cbShaderType][bindIndex] != cb.buffer.Get())) { \
			ctx->dxCall(bindIndex, 1, cb.buffer.GetAddressOf()); \
			m_ActiveBuffers[cbShaderType][bindIndex] = cb.buffer.Get(); \
		}

        BIND_CB(kShaderStageVertex, VSSetConstantBuffers); //kShaderStageVertex
        BIND_CB(kShaderStageFragment, PSSetConstantBuffers); //kShaderStageFragment
        cb.dirty = false;
    }

}

UniformBuffers &GetUniformBuffers() {
    static UniformBuffers uniformBuffers;
    return uniformBuffers;
}

}// namespace AN::D3D11