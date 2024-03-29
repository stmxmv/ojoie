//
// Created by Aleudillonam on 5/2/2023.
//

#include "FBXImporter.hpp"
#include "ojoie/Asset/ImportMesh.hpp"
#include "ojoie/Core/Exception.hpp"
#include  "ojoie/Math/Math.hpp"
#include <ojoie/Utility/Log.h>

#include <algorithm>
#include <ranges>
#include <utility>

#if AN_DEBUG
#include <iostream>
using std::cout, std::endl;
#endif

extern "C"
{

    __declspec(dllexport) AN::FBXImporterImpl *ANCreateFBXImporter(void)
    {
        return new ANPlugin::FBXImporter();
    }

    __declspec(dllexport) void ANDeleteFBXImporter(AN::FBXImporterImpl *importer)
    {
        delete importer;
    }
}


namespace ANPlugin
{

using namespace AN;

#define MAX_BONE_PER_VERTEX 32
struct VertexInfo
{
    Vector3f position;
    Vector2f texcoord0;
    Vector2f texcoord1;
    Vector3f normal;
    Vector4f tangent;

    Vector2f weight[MAX_BONE_PER_VERTEX]; // x is index to bone list, y is the weight
    int      weightNum;
};

struct IndexInfo
{
    UInt32 indices[3];
    UInt32 materialIndex;
};

template<typename KeyType, typename ValueType>
struct UnorderedMapGenerator
{
    struct Hash
    {
        uint32_t operator() (const KeyType &a) const
        {
            uint32_t digest = 0;
            for (size_t i = 0; i < sizeof(a); i++)
                digest = _mm_crc32_u8(digest, ((uint8_t *) &a)[i]);
            return digest;
        }
    };
    struct CompareEq
    {
        bool operator() (const KeyType &a, const KeyType &b) const
        {
            return !memcmp(&a, &b, sizeof(a));
        }
    };
    typedef std::unordered_map<KeyType, ValueType, Hash, CompareEq> Type;
};

// Get the value of a geometry element for a triangle vertex
template<typename TGeometryElement, typename TValue>
TValue GetVertexElement(TGeometryElement *pElement, int iPoint, int iTriangle, int iVertex, TValue defaultValue)
{
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

Quaternionf ConvertEulerAngles(FbxDouble3 eulerAngles, int rotationOrder)
{
    if (rotationOrder == eEulerYXZ)
    {
        return Math::toQuat(Math::eulerAngleYXZ(Math::radians(eulerAngles[0]), Math::radians(eulerAngles[1]), Math::radians(eulerAngles[2])));
    }
    else
    {
        // convert rotation order

        if (rotationOrder == eEulerXYZ)
        {
           return Math::toQuat(Math::eulerAngleZYX(Math::radians(eulerAngles[2]), Math::radians(eulerAngles[1]), Math::radians(eulerAngles[0])));
        }
        else
        {
            AN_LOG(Error, "Unsupported rotation order");
        }
    }

    return Math::identity<Quaternionf>();
}


FBXImporter::FBXImporter()
{
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

FBXImporter::~FBXImporter()
{
    mCallback->Destroy();
    ios->Destroy();
    _fbxManager->Destroy();
}

FbxCallback::State FBXImporter::OnEmbeddedFileRead(void *pUserData, FbxClassId pDataHint, const char *pFileName, const void *pFileBuffer, size_t pSizeInBytes)
{
    // we only want to process bitmaps, hence, the hint must be FbxVideo::ClassId
    // Any other type will be processd with the FBX SDK default behaviour.
    if (pDataHint != FbxVideo::ClassId)
        return FbxCallback::eNotHandled;
    if (!pFileBuffer || pSizeInBytes == 0)
        return FbxCallback::eFailure;
    FBXImporter *importer = reinterpret_cast<FBXImporter *>(pUserData);
    if (importer)
    {
        auto it = importer->m_embeddedResources.find(pFileName);
        if (it != importer->m_embeddedResources.end())
        {
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

bool FBXImporter::importMesh_traverse(UInt32 &meshIndex, FbxNode *node, AN::Error *error)
{
//    // try to extract bone
//    bool isBone = false;
//    if (node->GetNodeAttribute() &&
//        node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
//    {
//        // This is a bone
//        FbxSkeleton *skeleton = node->GetSkeleton();
//        // Extract bone information here
//        // Extract bone name
//        FbxString boneName = node->GetName();
//        FbxString parentName;
//        if (skeleton->IsSkeletonRoot())
//        {
//            parentName = "Skeleton Root";
//        }
//        else
//        {
//            parentName = node->GetParent()->GetName();
//        }
//
//        // Extract local transformation
//        FbxDouble3 translation = node->LclTranslation.Get();
//        FbxVector4 rotation = node->LclRotation.Get();
//        FbxVector4 scaling = node->LclScaling.Get();
//
//#if AN_DEBUG
//        cout << "Found Bone:  " << boneName << " parent: " << parentName << "  localPosition: "
//             << std::format("({}, {}, {})", translation[0], translation[1], translation[2]) << endl;
//#endif
//
//        isBone = true;
//    }
//
//    if (isBone)
//    {
//        for (int j = 0; j < node->GetChildCount(); j++)
//        {
//            importMesh_traverse(meshIndex, node->GetChild(j), error);
//        }
//        return true;
//    }

    if (node->GetMesh() && node->GetMesh()->GetPolygonCount() != 0)
    {///reject zero vertex mesh

        if (_importMeshes.size() < meshIndex + 1)
        {
            _importMeshes.resize(meshIndex + 1);
        }

        ImportMesh &importMesh = _importMeshes[meshIndex];

        Vector3f meshScale = Vector3f(1.f, 1.f, 1.f);

        {
            auto translation = node->LclTranslation.Get();
            importMesh.localPosition = Vector3f(translation[0], translation[1], translation[2]); // bone unit is cm
                                                                                                //            importMesh.localRotation = ConvertEulerAngles(node->LclRotation.Get(), (int)node->RotationOrder.Get());
            importMesh.localRotation = Math::identity<Quaternionf>();
            auto scale = node->LclScaling.Get();
            importMesh.localScale = Vector3f(scale[0], scale[1], scale[2]);

            meshScale = importMesh.localScale;
        }

        // Do something with mesh
        FbxMesh *mesh = node->GetMesh();
        int numDeformers = mesh->GetDeformerCount(FbxDeformer::eSkin);

        FbxSkin* skin = nullptr;
        std::vector<std::string> boneNameList;

        std::vector<std::vector<Vector2f>> boneWeights;

        if (numDeformers > 0)
        {
            /// FIXME currently we just use one deformer
            skin = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);

            if (skin != nullptr)
            {
                boneWeights.resize(mesh->GetControlPointsCount());

                for (int clusterIndex = 0; clusterIndex < skin->GetClusterCount(); ++clusterIndex)
                {
                    FbxCluster *cluster    = skin->GetCluster(clusterIndex);
                    FbxNode    *linkedBone = cluster->GetLink();

                    boneNameList.emplace_back(linkedBone->GetName());

                    // Process each vertex
                    for (int i = 0; i < cluster->GetControlPointIndicesCount(); ++i) {
                        int controlPointIndex = cluster->GetControlPointIndices()[i];
                        double weight = cluster->GetControlPointWeights()[i];

                        if (boneWeights[controlPointIndex].size() == MAX_BONE_PER_VERTEX)
                        {
                            AN_LOG(Warning, "exceed MAX_BONE_PER_VERTEX");
                            continue;
                        }

                        boneWeights[controlPointIndex].emplace_back(clusterIndex, weight);
                        // Here you have the bone (linkedBone), the vertex index (controlPointIndex), and the weight
                        // Process this information as needed
                    }
                }

                importMesh.bones.resize(boneNameList.size());
                for (int clusterIndex = 0; clusterIndex < skin->GetClusterCount(); ++clusterIndex)
                {
                    FbxCluster *cluster    = skin->GetCluster(clusterIndex);
                    FbxNode    *linkedBone = cluster->GetLink();

                    importMesh.bones[clusterIndex].name = boneNameList[clusterIndex];

                    FbxDouble3 translation = linkedBone->LclTranslation.Get();
                    importMesh.bones[clusterIndex].localPosition = Vector3f(translation[0], translation[1], translation[2]) * (float)scaleFactor; // bone unit is cm

                    FbxDouble3 eulerAngles = linkedBone->LclRotation.Get();

                    int rotationOrder = linkedBone->RotationOrder.Get();
                    if (rotationOrder == eEulerYXZ)
                    {
                        importMesh.bones[clusterIndex].localRotation = Math::toQuat(Math::eulerAngleYXZ(Math::radians(eulerAngles[0]), Math::radians(eulerAngles[1]), Math::radians(eulerAngles[2])));
                    }
                    else
                    {
                        // convert rotation order

                        if (rotationOrder == eEulerXYZ)
                        {
                            importMesh.bones[clusterIndex].localRotation = Math::toQuat(Math::eulerAngleZYX(Math::radians(eulerAngles[2]), Math::radians(eulerAngles[1]), Math::radians(eulerAngles[0])));
                        }
                        else
                        {
                            AN_LOG(Error, "Unsupported rotation order");
                        }
                    }


                    if (linkedBone->GetSkeleton()->IsSkeletonRoot())
                    {
                        importMesh.bones[clusterIndex].parent = -1;

                        FbxNode *parent = linkedBone->GetParent();
                        if (parent)
                        {
                            ImportBone rootBone{};
                            FbxNode *rootNode = parent;

                            translation = rootNode->LclTranslation.Get();
                            auto scale = rootNode->LclScaling.Get();

                            rootBone.localPosition = Vector3f(translation[0], translation[1], translation[2]) * (float)scaleFactor; // bone unit is cm
                            rootBone.localScale = Vector3f(scale[0], scale[1], scale[2]);
                            eulerAngles = rootNode->LclRotation.Get();

                            rotationOrder = rootNode->RotationOrder.Get();
                            if (rotationOrder == eEulerYXZ)
                            {
                                rootBone.localRotation = Math::toQuat(Math::eulerAngleYXZ(Math::radians(eulerAngles[0]), Math::radians(eulerAngles[1]), Math::radians(eulerAngles[2])));
                            }
                            else
                            {
                                // convert rotation order

                                if (rotationOrder == eEulerXYZ)
                                {
                                    rootBone.localRotation = Math::toQuat(Math::eulerAngleZYX(Math::radians(eulerAngles[2]), Math::radians(eulerAngles[1]), Math::radians(eulerAngles[0])));
                                }
                                else
                                {
                                    AN_LOG(Error, "Unsupported rotation order");
                                }
                            }

                            _importMeshes[meshIndex].rootBone = rootBone;
                        }

                    }
                    else
                    {
                        importMesh.bones[clusterIndex].parent =
                                std::find(boneNameList.begin(), boneNameList.end(),
                                          linkedBone->GetParent()->GetName()) - boneNameList.begin();
                    }
                }

#if AN_DEBUG
                cout << "\nBone names" << endl;
                for (const auto &name : boneNameList)
                {
                    cout << name << endl;
                }
                cout << endl;
#endif
            }

            FbxBlendShape *blendShape = (FbxBlendShape *)mesh->GetDeformer(0, FbxDeformer::eBlendShape);

            if (blendShape != nullptr)
            {
                for (int channelIndex = 0; channelIndex < blendShape->GetBlendShapeChannelCount(); ++channelIndex) {
                    FbxBlendShapeChannel* channel = blendShape->GetBlendShapeChannel(channelIndex);
                    if (channel != nullptr) {
                        const char* channelName = channel->GetName();
                        std::cout << "Channel Name: " << channelName << std::endl;

                        // Iterate through all target shapes of this channel
                        for (int targetShapeIndex = 0; targetShapeIndex < channel->GetTargetShapeCount(); ++targetShapeIndex) {
                            FbxShape* targetShape = channel->GetTargetShape(targetShapeIndex);
                            if (targetShape != nullptr) {
                                // Get target shape name
                                const char* targetShapeName = targetShape->GetName();
                                std::cout << "  Target Shape Name: " << targetShapeName << std::endl;

                                // Influence percentage (assuming one target shape per channel)
                                double influence = channel->DeformPercent.Get();
                                std::cout << "  Influence: " << influence << "%" << std::endl;

                                // Access vertex deltas here (if needed)
                                int meshControlPointsCount = mesh->GetControlPointsCount();
                                int targetShapeControlPointsCount = targetShape->GetControlPointsCount();

                                if (meshControlPointsCount != targetShapeControlPointsCount) {
                                    std::cerr << "Warning: Mesh and target shape control points count do not match." << std::endl;
                                    continue;
                                }

                                FbxVector4* meshControlPoints = mesh->GetControlPoints();
                                FbxVector4* targetShapeControlPoints = targetShape->GetControlPoints();

                                std::vector<Vector3f> vertexDeltas(meshControlPointsCount);
                                int deltaCount = 0;
                                for (int i = 0; i < meshControlPointsCount; ++i) {
                                    FbxVector4 fbxDelta = targetShapeControlPoints[i] - meshControlPoints[i];

                                    Vector3f delta = Vector3f(fbxDelta[0], -fbxDelta[2], fbxDelta[1]);

                                    if (!CompareApproximately(Math::length(delta), 0.f))
                                    {
                                        ++deltaCount;
                                    }
                                }
                                cout << "       deltaCount: " << deltaCount << endl;

                                // get normal and tangent

                            }
                        }
                    }
                }
            }
        }



        int numSubMeshes = mesh->GetNode()->GetMaterialCount();

        bool hasMat = true;
        if (numSubMeshes == 0)
        {
            numSubMeshes = 1;
            hasMat       = false;
        }
        _importMeshes[meshIndex].subMeshes.resize(numSubMeshes);
        _importMeshes[meshIndex].subMeshes[0].material.importTextureID = -1;

        if (hasMat)
        {
            for (int i = 0; i < numSubMeshes; ++i)
            {
                /// assume no material texture found
                _importMeshes[meshIndex].subMeshes[i].material.importTextureID = -1;

                FbxSurfaceMaterial *material = mesh->GetNode()->GetMaterial(i);

                FbxProperty first = material->GetFirstProperty();

                while (first.IsValid())
                {
                    AN_LOG(Log, "prop name %s", (const char *) first.GetName());
                    first = material->GetNextProperty(first);
                }

                FbxProperty mayaProp = material->FindProperty("Maya");
                FbxProperty prop     = mayaProp.Find("baseColor");
                if (prop.IsValid())
                {
                    /// we don't get the layered texture
                    // Directly get textures
                    //                int subObjectCount = prop.GetSrcObjectCount();
                    //                for (int j = 0; j < subObjectCount; ++j) {
                    //                    FbxObject *srcObject = prop.GetSrcObject(j);
                    //                    AN_LOG(Log, "BaseColor subObject %s", srcObject->GetName());
                    //                }

                    int textureCount = prop.GetSrcObjectCount<FbxTexture>();
                    if (textureCount > 0)
                    {
                        /// we just get the first texture
                        FbxTexture *texture = FbxCast<FbxTexture>(prop.GetSrcObject<FbxTexture>(0));
                        // Then, you can get all the properties of the texture, include its name
                        const char     *textureName = texture->GetName();
                        FbxProperty     p           = texture->RootProperty.Find("Filename");
                        FbxFileTexture *fileTexture = FbxCast<FbxFileTexture>(texture);

                        if (fileTexture)
                        {
                            if (auto it = m_embeddedResources.find(fileTexture->GetFileName());
                                it != m_embeddedResources.end())
                            {

                                UInt8 *data = it->second.first.get();

                                auto it2 = std::find_if(_textureIDMap.begin(), _textureIDMap.end(), [data](auto &&pair)
                                                        { return pair.second.first == data; });

                                if (it2 != _textureIDMap.end())
                                {
                                    _importMeshes[meshIndex].subMeshes[i].material.importTextureID = it2->first;
                                }
                                else
                                {
                                    /// assign texture id
                                    UInt32 id = 0;
                                    if (_textureIDMap.empty())
                                    {
                                        _textureIDMap[id] = std::make_pair(it->second.first.get(), it->second.second);
                                    }
                                    else
                                    {
                                        id                = (--_textureIDMap.end())->first + 1;
                                        _textureIDMap[id] = std::make_pair(it->second.first.get(), it->second.second);
                                    }
                                    _importMeshes[meshIndex].subMeshes[i].material.importTextureID = id;
                                }
                            }
                        }
                    }
                    else
                    {
                        //                    textureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();
                        //                    if (textureCount > 0) {
                        //                        FbxLayeredTexture *texture = FbxCast<FbxLayeredTexture>(prop.GetSrcObject<FbxLayeredTexture>(0));
                        //                    }
                    }
                }
            }
        }


        const FbxGeometryElementNormal *pNormals = mesh->GetElementNormal(0);
        if (!pNormals)
        {
            AN_LOG(Info, "generate normals");
            // Generate normals if we don't have any
            mesh->GenerateNormals();
            pNormals = mesh->GetElementNormal(0);
        }

        const FbxGeometryElementTangent *pTangents = mesh->GetElementTangent(0);
        if (!pTangents)
        {
            AN_LOG(Info, "generate tangents");
            mesh->GenerateTangentsData(0);
            pTangents = mesh->GetElementTangent(0);
        }

        const FbxGeometryElementUV *pUVs    = mesh->GetElementUV(0);
        int                         uvCount = mesh->GetElementUVCount();

        const FbxLayerElementMaterial *pPolygonMaterials = mesh->GetElementMaterial();

        FbxGeometryElement::EMappingMode mappingMode{};
        if (pPolygonMaterials)
        {
            assert(pPolygonMaterials->GetReferenceMode() == FbxGeometryElement::eIndex ||
                   pPolygonMaterials->GetReferenceMode() == FbxGeometryElement::eIndexToDirect);
            mappingMode = pPolygonMaterials->GetMappingMode();
        }

        auto getMaterialIndex = [pPolygonMaterials, mappingMode, numSubMeshes](uint32_t triangleIndex)
        {
            if (pPolygonMaterials == nullptr) return 0U;
            UInt32 lookupIndex = 0;
            switch (mappingMode)
            {
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

        for (uint32_t t = 0; t < numPolygons; t++)
        {
            uint32_t indices[3];
            for (uint32_t v = 0; v < 3; v++)
            {
                int iPoint = mesh->GetPolygonVertex(t, v);

                FbxVector4 point   = mesh->GetControlPointAt(iPoint);
                FbxVector4 normal  = GetVertexElement(pNormals, iPoint, t, v, FbxVector4(0, 0, 0, 0));
                FbxVector2 uv      = GetVertexElement(pUVs, iPoint, t, v, FbxVector2(0, 1));
                FbxVector4 tangent = GetVertexElement(pTangents, iPoint, t, v, FbxVector4(0, 0, 0, 0));

                VertexInfo vertex = {};
                vertex.position   = Vector3f(float(point[0]), -float(point[2]), float(point[1])) * meshScale * (float) scaleFactor;
                vertex.normal     = Vector3f(float(normal[0]), -float(normal[2]), float(normal[1]));
                vertex.texcoord0  = Vector2f(float(uv[0]), 1.0f - float(uv[1]));
                vertex.tangent    = Vector4f(tangent[0], -tangent[2], tangent[1], tangent[3]);

                /// get bone weights
                if (skin != nullptr)
                {
                    for (const Vector2f &w : boneWeights[iPoint])
                    {
                        vertex.weight[vertex.weightNum++] = w;
                    }
                }

                auto it = hashMap.find(vertex);
                if (it != hashMap.end())
                {
                    // it's a duplicate vertex
                    indices[v] = it->second;
                }
                else
                {
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
        std::sort(triIndices.begin(), triIndices.end(), [](auto &&a, auto &&b)
                  { return a.materialIndex < b.materialIndex; });


        for (VertexInfo &info : vertices)
        {
            importMesh.positions.push_back(info.position);
            importMesh.texcoords[0].push_back(info.texcoord0);
            if (uvCount > 1)
            {
                importMesh.texcoords[1].push_back(info.texcoord1);
            }
            importMesh.normals.push_back(info.normal);
            importMesh.tangents.push_back(info.tangent);

            importMesh.boneWeights.push_back(std::vector<Vector2f>(info.weight, info.weight + info.weightNum));
        }


        for (int i = 0; i < numSubMeshes; ++i)
        {
            auto indexBegin = std::ranges::find_if(triIndices, [i](auto &&info)
                                                   { return info.materialIndex == i; });

            ANAssert(indexBegin != triIndices.end());

            auto indexEnd = std::ranges::find_if_not(indexBegin, triIndices.end(), [i](auto &&info)
                                                     { return info.materialIndex == i; });

            for (const IndexInfo &info : std::ranges::subrange(indexBegin, indexEnd))
            {
                importMesh.subMeshes[i].indices.insert(importMesh.subMeshes[i].indices.end(),
                                                       std::ranges::begin(info.indices),
                                                       std::ranges::end(info.indices));
            }
        }

        ++meshIndex;
    }

    for (int i = 0; i < node->GetChildCount(); i++)
    {
        if (!importMesh_traverse(meshIndex, node->GetChild(i), error))
            return false;
    }

    return true;
}

bool FBXImporter::importMesh(AN::Error *error)
{
    if (_scene == nullptr)
    {
        *error = AN::Error(1, "scene not loaded");
        return false;
    }
    UInt32 meshIndex = 0;
    return importMesh_traverse(meshIndex, _scene->GetRootNode(), error);
}

bool FBXImporter::loadScene(const char *filePath, AN::Error *error)
{
    if (_scene)
    {
        _scene->Destroy();
        _scene = nullptr;
    }

    /// clear last import
    _importMeshes.clear();
    _textureIDMap.clear();
    m_embeddedResources.clear();

    FbxAutoDestroyPtr<FbxImporter> fbxImporter(FbxImporter::Create(_fbxManager, ""));
    FbxAutoDestroyPtr<FbxScene>    lScene(FbxScene::Create(_fbxManager, ""));

    const bool lImportStatus = fbxImporter->Initialize(filePath, -1, fbxImporter->GetIOSettings());

    if (lImportStatus)
    {
        // SetEmbeddedFileReadCallback needs to be called after Initialize()
        // because the Reset() function will clear it (but before Import()).
        fbxImporter->SetEmbeddedFileReadCallback(mCallback);
        fbxImporter->Import(lScene);

        FbxAxisSystem sceneAxisSystem = lScene->GetGlobalSettings().GetAxisSystem();
        FbxAxisSystem ourAxisSystem( FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded );

        if(sceneAxisSystem != ourAxisSystem)
        {
            ourAxisSystem.ConvertScene(lScene);
        }

        // instead of converting to meters, user should manual set the scale factor
        if (lScene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::m)
        {
            scaleFactor *= lScene->GetGlobalSettings().GetSystemUnit().GetConversionFactorTo(FbxSystemUnit::m);
        }

        FbxSystemUnit sceneSystemUnit = lScene->GetGlobalSettings().GetSystemUnit();
        if (sceneSystemUnit.GetScaleFactor() != 1.0)
        {
            FbxSystemUnit ourSystemUnit(1.0);
            ourSystemUnit.ConvertScene(lScene);
        }

        // assume fbx file unit is cm, convert to m
//        scaleFactor *= 0.01;

        // triangulate
        FbxGeometryConverter GeometryConverter(_fbxManager);
        if (!GeometryConverter.Triangulate(lScene, true, false))
        {
            if (error)
            {
                *error = AN::Error(1, "mesh cannot triangulate");
            }
            return false;
        }

        GeometryConverter.RemoveBadPolygonsFromMeshes(lScene);
        _scene = lScene.Release();
    }
    else
    {
        return false;
    }
    return true;
}

std::span<const AN::ImportMesh> FBXImporter::getImportMeshes()
{
    return _importMeshes;
}

UInt8 *FBXImporter::getTextureData(UInt32 id, UInt64 &size)
{
    if (auto it = _textureIDMap.find(id); it != _textureIDMap.end())
    {
        size = it->second.second;
        return it->second.first;
    }
    return nullptr;
}


}// namespace ANPlugin