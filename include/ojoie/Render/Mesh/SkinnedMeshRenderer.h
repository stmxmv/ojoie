//
// Created by aojoie on 12/16/2023.
//

#pragma once

#include <ojoie/Components/Transform.hpp>
#include <ojoie/Core/Component.hpp>
#include <ojoie/Render/Material.hpp>
#include <ojoie/Render/Mesh/Mesh.hpp>
#include <ojoie/Render/Renderer.hpp>

namespace AN
{

class AN_API SkinnedMeshRenderer : public Renderer
{
    Mesh *m_Mesh;
    int        m_RootBoneIndex;
    Transform *m_RootBone;
    std::vector<Transform *> m_Bones;
    //  not serialized

    struct TransformData {
        Matrix4x4f objectToWorld;
        Matrix4x4f worldToObject;
    };

    TransformData m_TransformData[kMaxFrameInFlight];

    VertexData   m_VertexData;
    VertexBuffer m_VertexBuffer;

    AN_CLASS(SkinnedMeshRenderer, Renderer)
    AN_OBJECT_SERIALIZE(SkinnedMeshRenderer)

    void InitVertexBuffer();

public:

    explicit SkinnedMeshRenderer(ObjectCreationMode mode);

    virtual bool init() override;
    virtual bool initAfterDecode() override;

    Mesh *GetMesh() const { return m_Mesh; }
    void SetMesh(Mesh *mesh);

    void SetBones(const std::vector<Transform *> &bones);

    Transform *GetRootBone();
    int        GetRootBoneIndex();

    virtual void Update(UInt32 frameIndex) override;
    virtual void Render(RenderContext &renderContext, const char *pass) override;
};


}
