//
// Created by Aleudillonam on 5/2/2023.
//

#ifndef OJOIE_AN_PLUGIN_FBXIMPORTER_HPP
#define OJOIE_AN_PLUGIN_FBXIMPORTER_HPP

#include <ojoie/Asset/ImportMesh.hpp>
#include <ojoie/Asset/FBXImporter.hpp>

#include <fbxsdk.h>

#include <unordered_map>
#include <string>
#include <map>

namespace ANPlugin {

class FBXImporter : public AN::FBXImporterImpl {

    double scaleFactor = 1.0;
    FbxManager *_fbxManager;
    FbxIOSettings *ios;
    std::unordered_map<std::string, std::pair<std::unique_ptr<UInt8 []>, size_t>> m_embeddedResources;
    std::map<UInt32, std::pair<UInt8 *, size_t>> _textureIDMap;
    FbxEmbeddedFileCallback* mCallback;

    FbxScene *_scene{};

    std::vector<AN::ImportMesh> _importMeshes;

    bool importMesh_traverse(UInt32 &meshIndex, FbxNode *node, AN::Error *error);

public:

    FBXImporter();

    virtual ~FBXImporter() override;

    virtual bool loadScene(const char *filePath, AN::Error *error) override;

    virtual bool importMesh(AN::Error *error) override;

    virtual std::span<const AN::ImportMesh> getImportMeshes() override;

    virtual UInt8                          *getTextureData(UInt32 id, UInt64 &size) override;

    // Callback function
    static FbxCallback::State OnEmbeddedFileRead(void* pUserData, FbxClassId pDataHint,
                                                 const char* pFileName, const void* pFileBuffer, size_t pSizeInBytes);

};


}




#endif//OJOIE_AN_PLUGIN_FBXIMPORTER_HPP
