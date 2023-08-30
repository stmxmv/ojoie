//
// Created by aojoie on 5/24/2023.
//

#pragma once
#include <ojoie/Render/CommandBuffer.hpp>
#include <ojoie/Render/UniformBuffers.hpp>
#include <ojoie/Render/private/D3D11/Common.hpp>

#include <vector>

namespace AN::D3D11 {

class UniformBuffers : public AN::UniformBuffers {

    struct ConstBuffer {
        int                  bindIndex[kShaderStageCount];
        unsigned             bindStages;
        bool                 dirty;
        std::vector<UInt8>  data;
        ComPtr<ID3D11Buffer> buffer;
    };

    typedef std::vector<UInt32>      ConstBufferKeys;
    typedef std::vector<ConstBuffer> ConstBuffers;

    ConstBufferKeys m_BufferKeys;
    ConstBuffers    m_Buffers;

    ID3D11Buffer*	m_ActiveBuffers[kShaderStageCount][16];

public:

    void destroyAll();

    void setBufferInfo(int id, int size);

    int  findAndBind(int id, ShaderStage shaderType, int bind, int size);

    void setConstant(int index, int offset, const void* data, int size);

    void updateBuffers(AN::CommandBuffer *commandBuffer);

    void resetBinds();

    void reset();

    virtual bool init() override { return true; }

    virtual void deinit() override {
        destroyAll();
    }

    virtual void update() override {}
};

UniformBuffers &GetUniformBuffers();

}// namespace AN::D3D11
