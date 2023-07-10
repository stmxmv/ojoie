//
// Created by Aleudillonam on 5/2/2023.
//

#include "FBXImporter.hpp"
#include "ojoie/Asset/ImportMesh.hpp"
#include "ojoie/Core/Exception.hpp"
#include <ojoie/Utility/Log.h>

#include <algorithm>
#include <utility>
#include <ranges>

extern "C" {

__declspec(dllexport)  AN::FBXImporterImpl *ANCreateFBXImporter(void) {
    return new ANPlugin::FBXImporter();
}

__declspec(dllexport) void ANDeleteFBXImporter(AN::FBXImporterImpl *importer) {
    delete importer;
}

}



namespace ANPlugin {

using namespace AN;

struct VertexInfo {
    Vector3f position;
    Vector2f texcoord0;
    Vector2f texcoord1;
    Vector3f normal;
    Vector4f tangent;
};

struct IndexInfo {
    UInt32 indices[3];
    UInt32 materialIndex;
};

template<typename KeyType, typename ValueType>
struct UnorderedMapGenerator {
    struct Hash {
        uint32_t operator() (const KeyType &a) const {
            uint32_t digest = 0;
            for (size_t i = 0; i < sizeof(a); i++)
                digest = _mm_crc32_u8(digest, ((uint8_t *) &a)[i]);
            return digest;
        }
    };
    struct CompareEq {
        bool operator() (const KeyType &a, const KeyType &b) const {
            return !memcmp(&a, &b, sizeof(a));
        }
    };
    typedef std::unordered_map<KeyType, ValueType, Hash, CompareEq> Type;
};

// Get the value of a geometry element for a triangle vertex
template<typename TGeometryElement, typename TValue>
TValue GetVertexElement(TGeometryElement *pElement, int iPoint, int iTriangle, int iVertex, TValue defaultValue) {
    if (!pElement || pElement->GetMappingMode() == FbxGeometryElement::eNone)
        return defaultValue;
    int index = 0;

    if (pElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
        index = iPoint;
    else if (pElement->GetMappingMode() == FbxGeometryElement::eByPolygon)
        index = iTriangle;
    else if (pElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
        index = iTriangle * 3 + iVertex;

    if (pElement->GetReferenceMode() != FbxGeometryElement::eDirect)
        index = pElement->GetIndexArray().GetAt(index);

    return pElement->GetDirectArray().GetAt(index);
}

FBXImporter::FBXImporter() {
    // Initialize the SDK manager. This object handles all our memory management.
    _fbxManager = FbxManager::Create();
    // Create the IO settings object.
    ios = FbxIOSettings::Create(_fbxManager, IOSROOT);
    _fbxManager->SetIOSettings(ios);
    ios->SetBoolProp(IMP_FBX_LINK, false);
    ios->SetBoolProp(IMP_FBX_GOBO, false);
    ios->SetBoolProp(IMP_FBX_SHAPE, false);
    ios->SetBoolProp(IMP_FBX_AUDIO, false);
    ios->SetBoolProp(IMP_FBX_CHARACTER, false);
    ios->SetBoolProp(IMP_FBX_CONSTRAINT, false);
    ios->SetBoolProp(IMP_FBX_POLYGROUP, false);


    mCallback = FbxEmbeddedFileCallback::Create(_fbxManager, "EmbeddedFileCallback");
    mCallback->RegisterReadFunction(OnEmbeddedFileRead, this);
}

FBXImporter::~FBXImporter() {
    mCallback->Destroy();
    ios->Destroy();
    _fbxManager->Destroy();
}

FbxCallback::State FBXImporter::OnEmbeddedFileRead(void *pUserData, FbxClassId pDataHint, const char *pFileName, const void *pFileBuffer, size_t pSizeInBytes) {
    // we only want to process bitmaps, hence, the hint must be FbxVideo::ClassId
    // Any other type will be processd with the FBX SDK default behaviour.
    if (pDataHint != FbxVideo::ClassId)
        return FbxCallback::eNotHandled;
    if (!pFileBuffer || pSizeInBytes == 0)
        return FbxCallback::eFailure;
    FBXImporter *importer = reinterpret_cast<FBXImporter *>(pUserData);
    if (importer) {
        auto it = importer->m_embeddedResources.find(pFileName);
        if (it != importer->m_embeddedResources.end()) {
            // pFileName has already been added to the map!
            return FbxCallback::eHandled;
        }
        // store the incoming bitmap buffer into our own memory space.
        const char              *bufferSrc = (const char *) (pFileBuffer);
        std::unique_ptr<UInt8[]> bufferDst = std::make_unique<UInt8[]>(pSizeInBytes);
        memcpy(bufferDst.get(), bufferSrc, pSizeInBytes);

        importer->m_embeddedResources.insert({ std::string(pFileName), { std::move(bufferDst), pSizeInBytes } });
        return FbxCallback::eHandled;
    }
    return FbxCallback::eFailure;
}

bool FBXImporter::importMesh_traverse(UInt32 &meshIndex, FbxNode *node, AN::Error *error) {
    if (node->GetMesh()) {

        if (_importMeshes.size() < meshIndex + 1) {
            _importMeshes.resize(meshIndex + 1);
        }

        // Do something with mesh
        FbxMesh *mesh = node->GetMesh();

        int numSubMeshes = mesh->GetNode()->GetMaterialCount();

        _importMeshes[meshIndex].subMeshes.resize(numSubMeshes);

        for (int i = 0; i < numSubMeshes; ++i) {
            /// assume no material texture found
            _importMeshes[meshIndex].subMeshes[i].material.importTextureID = -1;

            FbxSurfaceMaterial *material = mesh->GetNode()->GetMaterial(i);

            FbxProperty first = material->GetFirstProperty();

            while (first.IsValid()) {
                AN_LOG(Log, "prop name %s", (const char *)first.GetName());
                first = material->GetNextProperty(first);
            }

            FbxProperty         mayaProp = material->FindProperty("Maya");
            FbxProperty         prop     = mayaProp.Find("baseColor");
            if (prop.IsValid()) {
                /// we don't get the layered texture
                // Directly get textures
//                int subObjectCount = prop.GetSrcObjectCount();
//                for (int j = 0; j < subObjectCount; ++j) {
//                    FbxObject *srcObject = prop.GetSrcObject(j);
//                    AN_LOG(Log, "BaseColor subObject %s", srcObject->GetName());
//                }

                int textureCount = prop.GetSrcObjectCount<FbxTexture>();
                if (textureCount > 0) {
                    /// we just get the first texture
                    FbxTexture *texture = FbxCast<FbxTexture>(prop.GetSrcObject<FbxTexture>(0));
                    // Then, you can get all the properties of the texture, include its name
                    const char     *textureName = texture->GetName();
                    FbxProperty     p           = texture->RootProperty.Find("Filename");
                    FbxFileTexture *fileTexture = FbxCast<FbxFileTexture>(texture);

                    if (fileTexture) {
                        if (auto it = m_embeddedResources.find(fileTexture->GetFileName());
                            it != m_embeddedResources.end()) {

                            UInt8 *data = it->second.first.get();

                            auto it2 = std::find_if(_textureIDMap.begin(), _textureIDMap.end(), [data](auto &&pair) {
                                return pair.second.first == data;
                            });

                            if (it2 != _textureIDMap.end()) {
                                _importMeshes[meshIndex].subMeshes[i].material.importTextureID = it2->first;
                            } else {
                                /// assign texture id
                                UInt32 id = 0;
                                if (_textureIDMap.empty()) {
                                    _textureIDMap[id] = std::make_pair(it->second.first.get(), it->second.second);
                                } else {
                                    id                = (--_textureIDMap.end())->first + 1;
                                    _textureIDMap[id] = std::make_pair(it->second.first.get(), it->second.second);
                                }
                                _importMeshes[meshIndex].subMeshes[i].material.importTextureID = id;
                            }
                        }
                    }
                } else {
//                    textureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();
//                    if (textureCount > 0) {
//                        FbxLayeredTexture *texture = FbxCast<FbxLayeredTexture>(prop.GetSrcObject<FbxLayeredTexture>(0));
//                    }
                }
            }
        }

        const FbxGeometryElementNormal *pNormals = mesh->GetElementNormal(0);
        if (!pNormals) {
            AN_LOG(Info, "generate normals");
            // Generate normals if we don't have any
            mesh->GenerateNormals();
            pNormals = mesh->GetElementNormal(0);
        }

        const FbxGeometryElementTangent *pTangents = mesh->GetElementTangent(0);
        if (!pTangents) {
            AN_LOG(Info, "generate tangents");
            mesh->GenerateTangentsData(0);
            pTangents = mesh->GetElementTangent(0);
        }

        const FbxGeometryElementUV *pUVs    = mesh->GetElementUV(0);
        int                         uvCount = mesh->GetElementUVCount();

        const FbxLayerElementMaterial *pPolygonMaterials = mesh->GetElementMaterial();
        assert(pPolygonMaterials != nullptr);
        assert(pPolygonMaterials->GetReferenceMode() == FbxGeometryElement::eIndex ||
               pPolygonMaterials->GetReferenceMode() == FbxGeometryElement::eIndexToDirect);
        FbxGeometryElement::EMappingMode mappingMode      = pPolygonMaterials->GetMappingMode();
        auto                             getMaterialIndex = [pPolygonMaterials, mappingMode, numSubMeshes](uint32_t triangleIndex) {
            UInt32 lookupIndex = 0;
            switch (mappingMode) {
                case FbxGeometryElement::eByPolygon:
                    lookupIndex = triangleIndex;
                    break;
                case FbxGeometryElement::eAllSame:
                    lookupIndex = 0;
                    break;
                default:
                    assert(false);
                    break;
            }

            int materialIndex = pPolygonMaterials->mIndexArray->GetAt(lookupIndex);
            assert(materialIndex >= 0 && materialIndex < numSubMeshes);
            return uint32_t(materialIndex);
        };

        // vertex deduplication
        UnorderedMapGenerator<VertexInfo, uint32_t>::Type hashMap;
        int                                               numPolygons = mesh->GetPolygonCount();

        std::vector<VertexInfo> vertices;
        vertices.reserve(numPolygons * 3U);

        std::vector<IndexInfo> triIndices;
        triIndices.resize(numPolygons);

        for (uint32_t t = 0; t < numPolygons; t++) {
            uint32_t indices[3];
            for (uint32_t v = 0; v < 3; v++) {
                int iPoint = mesh->GetPolygonVertex(t, v);

                FbxVector4 point  = mesh->GetControlPointAt(iPoint);
                FbxVector4 normal = GetVertexElement(pNormals, iPoint, t, v, FbxVector4(0, 0, 0, 0));
                FbxVector2 uv     = GetVertexElement(pUVs, iPoint, t, v, FbxVector2(0, 1));
                FbxVector4 tangent = GetVertexElement(pTangents, iPoint, t, v, FbxVector4(0, 0, 0, 0));

                VertexInfo vertex = {};
                vertex.position   = Vector3f(float(point[0]), float(point[1]), float(point[2]));
                vertex.normal     = Vector3f(float(normal[0]), float(normal[1]), float(normal[2]));
                vertex.texcoord0  = Vector2f(float(uv[0]), 1.0f - float(uv[1]));
                vertex.tangent    = Vector4f(tangent[0], tangent[1], tangent[2], tangent[3]);

                auto it = hashMap.find(vertex);
                if (it != hashMap.end()) {
                    // it's a duplicate vertex
                    indices[v] = it->second;
                } else {
                    // we haven't run into this vertex yet
                    uint32_t index = uint32_t(vertices.size());
                    vertices.emplace_back(vertex);
                    hashMap[vertex] = index;
                    indices[v]      = index;
                }
            }

            uint32_t materialIndex = getMaterialIndex(t);

            IndexInfo &triShade    = triIndices[t];
            triShade.indices[0]    = indices[0];
            triShade.indices[1]    = indices[1];
            triShade.indices[2]    = indices[2];
            triShade.materialIndex = materialIndex;
        }

        /// sort index info by material index
        std::sort(triIndices.begin(), triIndices.end(), [](auto &&a, auto &&b) {
            return a.materialIndex < b.materialIndex;
        });

        ImportMesh &importMesh = _importMeshes[meshIndex];
        for (VertexInfo &info : vertices) {
            importMesh.positions.push_back(info.position);
            importMesh.texcoords[0].push_back(info.texcoord0);
            if (uvCount > 1) {
                importMesh.texcoords[1].push_back(info.texcoord1);
            }
            importMesh.normals.push_back(info.normal);
            importMesh.tangents.push_back(info.tangent);
        }


        for (int i = 0; i < numSubMeshes; ++i) {
            auto indexBegin = std::ranges::find_if(triIndices, [i](auto &&info) { return info.materialIndex == i; });

            ANAssert(indexBegin != triIndices.end());

            auto indexEnd = std::ranges::find_if_not(indexBegin, triIndices.end(), [i](auto &&info) { return info.materialIndex == i; });

            for (const IndexInfo &info : std::ranges::subrange(indexBegin, indexEnd)) {
                importMesh.subMeshes[i].indices.insert(importMesh.subMeshes[i].indices.end(),
                                                       std::ranges::begin(info.indices),
                                                       std::ranges::end(info.indices));

            }

        }

        ++meshIndex;
    }

    for (int i = 0; i < node->GetChildCount(); i++) {
        if (!importMesh_traverse(meshIndex, node->GetChild(i), error))
            return false;
    }

    return true;
}

bool FBXImporter::importMesh(AN::Error *error) {
    if (_scene == nullptr) {
        *error = AN::Error(1, "scene not loaded");
        return false;
    }
    UInt32 meshIndex = 0;
    return importMesh_traverse(meshIndex, _scene->GetRootNode(), error);
}

bool FBXImporter::loadScene(const char *filePath, AN::Error *error) {
    if (_scene) {
        _scene->Destroy();
        _scene = nullptr;
    }

    /// clear last import
    _importMeshes.clear();
    _textureIDMap.clear();
    m_embeddedResources.clear();

    FbxAutoDestroyPtr<FbxImporter> fbxImporter(FbxImporter::Create(_fbxManager, ""));
    FbxAutoDestroyPtr<FbxScene> lScene(FbxScene::Create(_fbxManager, ""));

    const bool   lImportStatus = fbxImporter->Initialize(filePath, -1, fbxImporter->GetIOSettings());

    if (lImportStatus) {
        // SetEmbeddedFileReadCallback needs to be called after Initialize()
        // because the Reset() function will clear it (but before Import()).
        fbxImporter->SetEmbeddedFileReadCallback(mCallback);
        fbxImporter->Import(lScene);

        // instead of converting to meters, user should manual set the scale factor
//        FbxSystemUnit::m.ConvertScene(lScene);
//        if (lScene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::m) {
//
//        }

        // triangulate
        FbxGeometryConverter GeometryConverter(_fbxManager);
        if (!GeometryConverter.Triangulate(lScene, true, false)) {
            if (error) {
                *error = AN::Error(1, "mesh cannot triangulate");
            }
            return false;
        }

        GeometryConverter.RemoveBadPolygonsFromMeshes(lScene);
        _scene = lScene.Release();

    } else {
        return false;
    }
    return true;
}

std::span<const AN::ImportMesh> FBXImporter::getImportMeshes() {
    return _importMeshes;
}

UInt8 *FBXImporter::getTextureData(UInt32 id, UInt64 &size) {
    if (auto it = _textureIDMap.find(id); it != _textureIDMap.end()) {
        size = it->second.second;
        return it->second.first;
    }
    return nullptr;
}


}