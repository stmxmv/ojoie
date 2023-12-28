//
// Created by Aleudillonam on 8/7/2022.
//

#ifndef OJOIE_MESH_HPP
#define OJOIE_MESH_HPP

#include <ojoie/Object/NamedObject.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Render/VertexBuffer.hpp>
#include <ojoie/Render/VertexData.hpp>
#include <vector>

namespace AN {

struct SubMesh {
    UInt32 indexCount;
    UInt32 indexOffset;

    AN_SERIALIZE_NO_IDPTR(SubMesh)
};

template<typename Coder>
void SubMesh::transfer(Coder &coder) {
    TRANSFER(indexCount);
    TRANSFER(indexOffset);
}

// TODO support bone influences more than 4
struct BoneWeight
{
    int   boneIndices[4];
    float weights[4];

    AN_SERIALIZE_NO_IDPTR(BoneWeight)
};

template<typename Coder>
void BoneWeight::transfer(Coder &coder) {
    TRANSFER(boneIndices);
    TRANSFER(weights);
}

class AN_API Mesh : public NamedObject {

    VertexData _vertexData;

    std::vector<SubMesh> _subMeshes;

    std::vector<UInt16> _indexBuffer;

    VertexBuffer _vertexBuffer;

    std::vector<Matrix4x4f> m_Bindposes;
    std::vector<BoneWeight> m_BoneWeights;

    AN_CLASS(Mesh, NamedObject)
    AN_OBJECT_SERIALIZE(Mesh)

public:
    explicit Mesh(ObjectCreationMode mode);

    virtual bool init() override;
    virtual bool initAfterDecode() override;

    virtual void dealloc() override;

    void createVertexBuffer();

    VertexBuffer &getVertexBuffer() { return _vertexBuffer; }

    SubMesh &getSubMesh(UInt32 index) { return _subMeshes[index]; }

    /// get the pipeline vertex descriptor
    VertexDescriptor getVertexDescriptor(std::span<const ShaderVertexInput> input) {
        return _vertexData.getVertexDescriptor(input);
    }

    const VertexStreamsLayout  &getStreamsLayout() const;
    const VertexChannelsLayout &getChannelsLayout() const;

    const ChannelInfo &GetChannel(int i) const { return _vertexData.getChannel(i); }
    const StreamInfo &GetStream(int i) const { return _vertexData.getStream(i); }

    bool   isAvailable(ShaderChannel channel) const { return _vertexData.hasChannel(channel); }

    const UInt8 *GetVertexDataPtr() const { return _vertexData.getDataPtr(); }
    UInt64 GetVertexDataSize() const { return _vertexData.getVertexSize(); }
    int    getVertexCount() const { return _vertexData.getVertexCount(); }

    UInt32 getAvailableChannels() const { return _vertexData.getChannelMask(); }

    UInt32 resizeVertices(size_t count, UInt32 shaderChannels,
                          const VertexStreamsLayout  &streams,
                          const VertexChannelsLayout &channels);

    UInt32 resizeVertices(size_t count, UInt32 shaderChannels) {
        return resizeVertices(count, shaderChannels, getStreamsLayout(), getChannelsLayout());
    }

    UInt32 getSubMeshCount() const { return _subMeshes.size(); }
    void setSubMeshCount(unsigned int count);


    void setVertices(Vector3f const *data, size_t count);
    void setNormals(Vector3f const *data, size_t count);
    void setTangents(Vector4f const *data, size_t count);
    void setUV(int uvIndex, Vector2f const *data, size_t count);
    bool setIndices(const UInt16* indices, unsigned count, unsigned submesh);

    void setBindposes(const Matrix4x4f *data, size_t count);
    void setBoneWeights(const BoneWeight *boneWeight, size_t count);

    UInt32 getIndicesCount() const { return _indexBuffer.size(); }
    UInt16 *getIndicesData() { return _indexBuffer.data(); }

    Matrix4x4f *getBindposesData() { return m_Bindposes.data(); }
    UInt32      getBindposesCount() const { return m_Bindposes.size(); }

    BoneWeight *getBoneWeightsData() { return m_BoneWeights.data(); }
    UInt32      getBoneWeightsCount() const { return m_BoneWeights.size(); }

    void initChannelsToDefault(unsigned begin, unsigned count, unsigned shaderChannels);

    // returns a bitmask of a newly created channels
    UInt32 formatVertices(UInt32 shaderChannels);

    // NOTE: make sure to call SetChannelDirty and RecalculateBounds when changing the geometry!
    StrideIterator<Vector3f> getVertexBegin() const { return _vertexData.MakeStrideIterator<Vector3f>(kShaderChannelVertex); }
    StrideIterator<Vector3f> getVertexEnd() const { return _vertexData.MakeEndIterator<Vector3f>(kShaderChannelVertex); }

    StrideIterator<Vector3f> getNormalBegin() const { return _vertexData.MakeStrideIterator<Vector3f>(kShaderChannelNormal); }
    StrideIterator<Vector3f> getNormalEnd() const { return _vertexData.MakeEndIterator<Vector3f>(kShaderChannelNormal); }

    //    StrideIterator<ColorRGBA32> GetColorBegin () const { return _vertexData.MakeStrideIterator<ColorRGBA32> (kShaderChannelColor); }
    //    StrideIterator<ColorRGBA32> GetColorEnd () const { return _vertexData.MakeEndIterator<ColorRGBA32> (kShaderChannelColor); }

    StrideIterator<Vector2f> getUvBegin(int uvIndex = 0) const { return _vertexData.MakeStrideIterator<Vector2f>((ShaderChannel) (kShaderChannelTexCoord0 + uvIndex)); }
    StrideIterator<Vector2f> getUvEnd(int uvIndex = 0) const { return _vertexData.MakeEndIterator<Vector2f>((ShaderChannel) (kShaderChannelTexCoord0 + uvIndex)); }

    StrideIterator<Vector4f> getTangentBegin() const { return _vertexData.MakeStrideIterator<Vector4f>(kShaderChannelTangent); }
    StrideIterator<Vector4f> getTangentEnd() const { return _vertexData.MakeEndIterator<Vector4f>(kShaderChannelTangent); }
};


}// namespace AN

#endif//OJOIE_MESH_HPP
