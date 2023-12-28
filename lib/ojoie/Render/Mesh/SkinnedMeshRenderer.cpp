//
// Created by aojoie on 12/16/2023.
//

#include "Render/Mesh/SkinnedMeshRenderer.h"
#include "Render/RenderContext.hpp"

#ifdef WITH_TBB
#include <tbb/parallel_for.h>
#endif


namespace AN
{

IMPLEMENT_AN_CLASS(SkinnedMeshRenderer)
LOAD_AN_CLASS(SkinnedMeshRenderer)
IMPLEMENT_AN_OBJECT_SERIALIZE(SkinnedMeshRenderer)
INSTANTIATE_TEMPLATE_TRANSFER(SkinnedMeshRenderer)

SkinnedMeshRenderer::SkinnedMeshRenderer(ObjectCreationMode mode) 
    : Super(mode),
      m_Mesh(),
      m_RootBone()
{
}

SkinnedMeshRenderer::~SkinnedMeshRenderer()
{
}

void SkinnedMeshRenderer::SetBones(const std::vector<Transform *> &bones)
{
    m_Bones = bones;
    GetRootBone();
}

Transform *SkinnedMeshRenderer::GetRootBone()
{
    if (m_RootBone)
    {
        return m_RootBone;
    }

    std::unordered_set<Transform *> boneSet(m_Bones.begin(), m_Bones.end());

    for (int i = 0; i < m_Bones.size(); ++i)
    {
        if (!boneSet.contains(m_Bones[i]->getParent()))
        {
            m_RootBone = m_Bones[i];
            m_RootBoneIndex = i;
            return m_RootBone;
        }
    }

    return nullptr;
}

int SkinnedMeshRenderer::GetRootBoneIndex()
{
    GetRootBone();
    return m_RootBoneIndex;
}

void SkinnedMeshRenderer::SetMesh(Mesh *mesh)
{
    m_Mesh = mesh;
    if (m_Mesh == nullptr)
    {
        return;
    }
    InitVertexBuffer();
}

void SkinnedMeshRenderer::Update(UInt32 frameIndex)
{
    if (!m_Mesh) return;

    Matrix4x4f localToWorld, worldToLocal;
    Vector3f position;
    Quaternionf  rotation;

    GetRootBone()->GetPositionAndRotation(position, rotation);
    localToWorld = Math::translate(position) * Math::toMat4(rotation);
    worldToLocal = Math::inverse(localToWorld);

    m_TransformData[frameIndex].objectToWorld = localToWorld;
    m_TransformData[frameIndex].worldToObject = worldToLocal;


    bool boneTransformChanged = false;
    for (int i = 0; i < m_Bones.size(); ++i)
    {
        if (m_Bones[i]->HasChanged())
        {
            boneTransformChanged = true;
            m_Bones[i]->SetHasChanged(false);
        }
    }

    if (!boneTransformChanged)
    {
        /// return early if bones transform not changed
        return;
    }

    /// FIXME do CPU skinning in manager

    /// cpu skinning
    const BoneWeight *boneWeights = m_Mesh->getBoneWeightsData();
    auto vertices = m_Mesh->getVertexBegin();
    auto normals = m_Mesh->getNormalBegin();
    auto tangents = m_Mesh->getTangentBegin();

//    Matrix4x4f rootPose = GetRootBone()->GetLocalToWorldMatrixNoScale();
    const Matrix4x4f *bindposes = m_Mesh->getBindposesData();

    std::vector<Matrix4x4f> skinningMatrices(m_Bones.size());

    Matrix4x4f rootPose = GetRootBone()->GetWorldToLocalMatrixNoScale();
    for (int i = 0; i < skinningMatrices.size(); ++i)
    {
        skinningMatrices[i] = rootPose * m_Bones[i]->getLocalToWorldMatrix() * bindposes[i];
    }

    auto positionData = m_VertexData.MakeStrideIterator<Vector3f>(kShaderChannelVertex);
    auto normalData = m_VertexData.MakeStrideIterator<Vector3f>(kShaderChannelNormal);
    auto tangentData = m_VertexData.MakeStrideIterator<Vector4f>(kShaderChannelTangent);

    auto skinMeshVertex = [&](int i)
    {
        const BoneWeight &boneWeight = boneWeights[i];
        Vector3f vertex = vertices[i];
        Vector3f normal = normals[i];
        Vector3f tangent = tangents[i];

        Vector3f resultPosition{}, resultNormal{}, resultTangent{};

        for (int j = 0; j < 4; ++j)
        {
            const Matrix4x4f &skinningMatrix = skinningMatrices[boneWeight.boneIndices[j]];
            float weight = boneWeight.weights[j];

            Vector3f pos0 = skinningMatrix * Vector4f(vertex, 1.f);
            Vector3f norm0 = skinningMatrix * Vector4f(normal, 0.f);
            Vector3f tan0 = skinningMatrix * Vector4f(tangent, 0.f);

            resultPosition += (pos0 * weight);
            resultNormal += (norm0 * weight);
            resultTangent += (tan0 * weight);
        }

        resultNormal = Math::normalize(resultNormal);
        resultTangent = Math::normalize(resultTangent);

        positionData[i] = resultPosition;
        normalData[i] = resultNormal;
        tangentData[i] = Vector4f(resultTangent, tangents[i].w);
    };

#ifdef WITH_TBB
    tbb::parallel_for(
            tbb::blocked_range<int>(0, m_Mesh->getVertexCount()),
            [&](const tbb::blocked_range<int>& range)
            {
                for (int i = range.begin(); i !=range.end(); ++i)
                {
                    skinMeshVertex(i);
                }
            });

#else
    for (int i = 0; i < m_Mesh->getVertexCount(); ++i)
    {
        skinMeshVertex(i);
    }
#endif


    VertexBufferData vertexBufferData;

    for (int i = 0; i < kShaderChannelCount; i++)
        vertexBufferData.channels[i] = m_Mesh->GetChannel(i);

    for (int i = 0; i < kMaxVertexStreams; i++)
        vertexBufferData.streams[i] = m_Mesh->GetStream(i);

    vertexBufferData.buffer      = m_VertexData.getDataPtr();
    vertexBufferData.bufferSize  = m_VertexData.getDataSize();
    vertexBufferData.vertexCount = m_VertexData.getVertexCount();

    m_VertexBuffer.updateVertexStream(vertexBufferData, 0);
}

void SkinnedMeshRenderer::Render(RenderContext &renderContext, const char *pass)
{
    if (m_Mesh == nullptr) return;
    
    for (int i = 0; i < m_Mesh->getSubMeshCount(); ++i) {
        SubMesh &subMesh = m_Mesh->getSubMesh(i);

        if (_materials.size() >= i + 1) {
            if (_materials[i] == nullptr) continue;
            Material &mat = *_materials[i];
            if (mat.hasPass(pass)) {
                /// setting per draw builtin properties
                mat.setVector("an_WorldTransformParams", { 0, 0, 0, 1.f });
                mat.setMatrix("an_ObjectToWorld", m_TransformData[renderContext.frameIndex].objectToWorld);
                mat.setMatrix("an_WorldToObject", m_TransformData[renderContext.frameIndex].worldToObject);

                mat.applyMaterial(renderContext.commandBuffer, pass);

                m_VertexBuffer.drawIndexed(renderContext.commandBuffer,
                                                     subMesh.indexCount,
                                                     subMesh.indexOffset,
                                                     0);
            }
        }
    }
}
bool SkinnedMeshRenderer::init()
{
    Super::init();
    return true;
}

bool SkinnedMeshRenderer::initAfterDecode()
{
    Super::initAfterDecode();
    if (m_Mesh != nullptr)
    {
        InitVertexBuffer();
    }
    return true;
}

void SkinnedMeshRenderer::InitVertexBuffer()
{
    VertexStreamsLayout layout =  { { kShaderChannelsHot, kShaderChannelsCold, 0, 0 } };
    m_VertexData.resize(m_Mesh->getVertexCount(),
                        (1 << kShaderChannelVertex) | (1 << kShaderChannelNormal) |
                        (1 << kShaderChannelTangent) | (1 << kShaderChannelTexCoord0),
                        layout, VertexDataInfo::kVertexChannelsDefault);

    strided_copy(m_Mesh->getVertexBegin(), m_Mesh->getVertexEnd(), m_VertexData.MakeStrideIterator<Vector3f>(kShaderChannelVertex));
    strided_copy(m_Mesh->getNormalBegin(), m_Mesh->getNormalEnd(), m_VertexData.MakeStrideIterator<Vector3f>(kShaderChannelNormal));
    strided_copy(m_Mesh->getUvBegin(), m_Mesh->getUvEnd(), m_VertexData.MakeStrideIterator<Vector2f>(kShaderChannelTexCoord0));
    strided_copy(m_Mesh->getTangentBegin(), m_Mesh->getTangentEnd(), m_VertexData.MakeStrideIterator<Vector4f>(kShaderChannelTangent));

    m_VertexBuffer.init();
    m_VertexBuffer.setVertexStreamMode(0, kStreamModeDynamic);
    m_VertexBuffer.setVertexStreamMode(1, kStreamModeNoAccess);

    IndexBufferData indexBufferData;
    indexBufferData.indices = m_Mesh->getIndicesData();
    indexBufferData.count = m_Mesh->getIndicesCount();


    VertexBufferData vertexBufferData;

    for (int i = 0; i < kShaderChannelCount; i++)
        vertexBufferData.channels[i] = m_Mesh->GetChannel(i);

    for (int i = 0; i < kMaxVertexStreams; i++)
        vertexBufferData.streams[i] = m_Mesh->GetStream(i);

    vertexBufferData.buffer      = m_VertexData.getDataPtr();
    vertexBufferData.bufferSize  = m_VertexData.getDataSize();
    vertexBufferData.vertexCount = m_VertexData.getVertexCount();

    m_VertexBuffer.updateVertexData(vertexBufferData);
    m_VertexBuffer.updateIndexData(indexBufferData);
}

template<typename _Coder>
void SkinnedMeshRenderer::transfer(_Coder &coder)
{
    Super::transfer(coder);
    TRANSFER(m_Mesh);
    TRANSFER(m_Bones);
}

}