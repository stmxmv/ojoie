//
// Created by Aleudillonam on 8/7/2022.
//
#include "Render/Mesh/Mesh.hpp"
#include "Render/Renderer.hpp"
#include "Threads/Dispatch.hpp"
#include <intrin.h>

namespace AN {

IMPLEMENT_AN_CLASS(Mesh);
LOAD_AN_CLASS(Mesh);

Mesh::Mesh(AN::ObjectCreationMode mode) : Super(mode) {}
Mesh::~Mesh() {
    /// default has 1 subMesh
    _subMeshes.resize(1);
}

bool Mesh::init() {
    if (!Super::init()) return false;
    return _vertexBuffer.init();
}

bool Mesh::initAfterDecode() {
    if (!Super::initAfterDecode()) return false;
    if (_vertexBuffer.init()) {
        createVertexBuffer();
        return true;
    }
    return false;
}

void Mesh::dealloc() {

    _vertexData.dealloc();
    _vertexBuffer.deinit();
    _indexBuffer.clear();
    _subMeshes.clear();
    Super::dealloc();
}


void Mesh::createVertexBuffer() {
    VertexBufferData vertexBuffer;

    for (int i = 0; i < kShaderChannelCount; i++)
        vertexBuffer.channels[i] = _vertexData.getChannel(i);

    for (int i = 0; i < kMaxVertexStreams; i++)
        vertexBuffer.streams[i] = _vertexData.getStream(i);

    vertexBuffer.buffer      = _vertexData.getDataPtr();
    vertexBuffer.bufferSize  = _vertexData.getDataSize();
    vertexBuffer.vertexCount = getVertexCount();


    IndexBufferData indexBuffer;
    indexBuffer.indices = _indexBuffer.data();
    indexBuffer.count   = _indexBuffer.size();

    _vertexBuffer.updateVertexData(vertexBuffer);
    _vertexBuffer.updateIndexData(indexBuffer);
}

const VertexStreamsLayout &Mesh::getStreamsLayout() const {
    return VertexData::kVertexStreamsDefault;
}

const VertexChannelsLayout &Mesh::getChannelsLayout() const {
    return VertexData::kVertexChannelsDefault;
}

UInt32 Mesh::formatVertices(UInt32 shaderChannels) {
    return resizeVertices(getVertexCount(), shaderChannels);
}

void Mesh::initChannelsToDefault(unsigned begin, unsigned count, unsigned shaderChannels) {
    if (shaderChannels & VERTEX_FORMAT1(Vertex))
        std::fill(getVertexBegin() + begin, getVertexBegin() + begin + count, Vector3f(0, 0, 0));
    if (shaderChannels & VERTEX_FORMAT1(Normal))
        std::fill(getNormalBegin() + begin, getNormalBegin() + begin + count, Vector3f(0, 0, 0));
    //    if (shaderChannels & VERTEX_FORMAT1(Color))
    //        std::fill (GetColorBegin () + begin, GetColorBegin () + begin + count, ColorRGBA32 (0xffffffff));
    if (shaderChannels & VERTEX_FORMAT1(TexCoord0))
        std::fill(getUvBegin(0) + begin, getUvBegin(0) + begin + count, Vector2f(0, 0));
    if (shaderChannels & VERTEX_FORMAT1(Tangent))
        std::fill(getTangentBegin() + begin, getTangentBegin() + begin + count, Vector4f(0, 0, 0, 0));

    if (shaderChannels & VERTEX_FORMAT1(TexCoord1)) {
        if (getAvailableChannels() & VERTEX_FORMAT1(TexCoord0))
            std::copy(getUvBegin(0) + begin, getUvBegin(0) + begin + count, getUvBegin(1) + begin);
        else
            std::fill(getUvBegin(1) + begin, getUvBegin(1) + begin + count, Vector2f(0, 0));
    }
}


UInt32 Mesh::resizeVertices(size_t count, UInt32 shaderChannels,
                            const VertexStreamsLayout  &streams,
                            const VertexChannelsLayout &channels) {
    ANAssert(count <= std::numeric_limits<UInt16>::max());

    UInt32 prevChannels = _vertexData.getChannelMask();

    if (_vertexData.getVertexCount() != count ||
        _vertexData.getChannelMask() != shaderChannels ||
        !_vertexData.conformsToStreamsLayout(streams) ||
        !_vertexData.conformsToChannelsLayout(channels)) {
        _vertexData.resize(count, shaderChannels, streams, channels);
    }

    return _vertexData.getChannelMask() & ~prevChannels;
}

void Mesh::setSubMeshCount(unsigned int count) {
    if (count == 0) {
        _indexBuffer.clear();
        _subMeshes.clear();
        return;
    }

    if (count < _subMeshes.size()) {
        _indexBuffer.resize(_subMeshes[count].indexOffset);
    }
    _subMeshes.resize(count);
}

void Mesh::setVertices(const Vector3f *data, size_t count) {
    if (count > std::numeric_limits<UInt16>::max()) {
        AN_LOG(Error, "Mesh.vertices is too large. A mesh may not have more than 65000 vertices.");
        return;
    }
    size_t prevCount = getVertexCount();

    if (prevCount != count) {
        unsigned prevChannels = getAvailableChannels();
        resizeVertices(count, prevChannels | VERTEX_FORMAT1(Vertex));

        // In case there were other channels present, initialize the newly created values of
        // the expanded buffer to something meaningful.
        if (prevCount != 0 && count > prevCount && (prevChannels & ~VERTEX_FORMAT1(Vertex))) {
            initChannelsToDefault(prevCount, count - prevCount, prevChannels & ~VERTEX_FORMAT1(Vertex));
        }
    }

    // Make sure we'll not be overrunning the buffer
    if (getVertexCount() < count)
        count = getVertexCount();

    strided_copy(data, data + count, getVertexBegin());
    //    SetChannelsDirty(VERTEX_FORMAT1(Vertex), false);

    // We do not recalc the bounds automatically when re-writing existing vertices
    //    if (prevCount != count)
    //        recalculateBounds();
}

void Mesh::setNormals(const Vector3f *data, size_t count) {
    if (count == 0 || !data) {
        formatVertices(getAvailableChannels() & ~VERTEX_FORMAT1(Normal));
        //        SetChannelsDirty (VERTEX_FORMAT1(Normal), false);
        return;
    }

    if (count != getVertexCount()) {
        AN_LOG(Error, "normals mismatch vertex num");
        return;
    }

    if (!isAvailable(kShaderChannelNormal))
        formatVertices(getAvailableChannels() | VERTEX_FORMAT1(Normal));

    strided_copy(data, data + count, getNormalBegin());

    //    SetChannelsDirty (VERTEX_FORMAT1(Normal), false);
}

void Mesh::setTangents(const Vector4f *data, size_t count) {
    if (count == 0 || !data) {
        formatVertices(getAvailableChannels() & ~VERTEX_FORMAT1(Tangent));
        //        SetChannelsDirty (VERTEX_FORMAT1(Tangent), false);
        return;
    }

    if (count != getVertexCount()) {
        AN_LOG(Error, "tangents mismatch vertex num");
        return;
    }

    if (!isAvailable(kShaderChannelTangent))
        formatVertices(getAvailableChannels() | VERTEX_FORMAT1(Tangent));

    strided_copy(data, data + count, getTangentBegin());
    //    SetChannelsDirty( VERTEX_FORMAT1(Tangent), false );
}

void Mesh::setUV(int uvIndex, const Vector2f *data, size_t count) {
    ShaderChannel texCoordChannel = static_cast<ShaderChannel>(kShaderChannelTexCoord0 + uvIndex);
    unsigned      texCoordMask    = 1 << texCoordChannel;
    if (count == 0 || !data) {
        formatVertices(getAvailableChannels() & ~texCoordMask);
        //        SetChannelsDirty (texCoordMask, false);
        return;
    }

    if (count != getVertexCount()) {
        const char *uvName = uvIndex == 1 ? "uv2" : "uv";
        AN_LOG(Error, "%s mismatch vertex num", uvName);
        return;
    }

    if (!isAvailable(texCoordChannel))
        formatVertices(getAvailableChannels() | texCoordMask);

    strided_copy(data, data + count, getUvBegin(uvIndex));
    //    SetChannelsDirty (texCoordMask, false);
}

bool Mesh::setIndices(const UInt16 *indices, unsigned int count, unsigned int submesh) {
    if (indices == NULL && count != 0) {
        ANAssert("failed setting indices. indices is NULL");
        return false;
    }

    if (submesh >= getSubMeshCount()) {
        AN_LOG(Error, "Failed setting triangles. Submesh index is out of bounds.");
        return false;
    }


    int oldCount = _subMeshes[submesh].indexCount;
    int oldOffset = _subMeshes[submesh].indexOffset;
    int diff = count - oldCount;

    if (diff > 0) {
        // growing
        _indexBuffer.insert(_indexBuffer.begin() + oldOffset + oldCount, diff, 0);

    } else {
        /// shrink
        _indexBuffer.erase(_indexBuffer.begin() + oldOffset, _indexBuffer.begin() + oldOffset - diff);
    }



    // Update the sub mesh
    _subMeshes[submesh].indexCount = count;

    // Synchronize subsequent sub meshes
    for (int i = submesh + 1; i < _subMeshes.size(); ++i) {
        _subMeshes[i].indexOffset = _subMeshes[i - 1].indexOffset + _subMeshes[i - 1].indexCount;
    }

    // Write indices into the allocated data
    memcpy(_indexBuffer.data() + oldOffset, indices, sizeof(UInt16) * count);

    return true;
}

template<typename _Coder>
void Mesh::transfer(_Coder &coder) {
    Super::transfer(coder);
    TRANSFER(_vertexData);
    TRANSFER(_subMeshes);
    TRANSFER(_indexBuffer);
}

IMPLEMENT_OBJECT_SERIALIZE(Mesh)



}// namespace AN