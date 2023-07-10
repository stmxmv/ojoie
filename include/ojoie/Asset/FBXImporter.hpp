//
// Created by Aleudillonam on 5/1/2023.
//

#ifndef OJOIE_AN_FBXIMPORTER_HPP
#define OJOIE_AN_FBXIMPORTER_HPP

#include <ojoie/Asset/ImportMesh.hpp>
#include <ojoie/Core/Exception.hpp>

#include <span>

namespace AN {

class FBXImporterImpl {
public:

    virtual ~FBXImporterImpl() = default;

    virtual bool loadScene(const char *filePath, Error *error) = 0;

    virtual bool importMesh(Error *error) = 0;

    virtual std::span<const AN::ImportMesh> getImportMeshes() = 0;

    virtual UInt8 *getTextureData(UInt32 id, UInt64 &size) = 0;

};

class AN_API FBXImporter {
    FBXImporterImpl *impl;
public:

    FBXImporter();

    ~FBXImporter();

    bool isValid() { return impl != nullptr; }

    bool loadScene(const char *filePath, Error *error) {
        return impl->loadScene(filePath, error);
    }

    bool importMesh(Error *error) {
        return impl->importMesh(error);
    }

    std::span<const AN::ImportMesh> getImportMeshes() {
        return impl->getImportMeshes();
    }

    UInt8 *getTextureData(UInt32 id, UInt64 &size) {
        return impl->getTextureData(id, size);
    }

};

}

#endif//OJOIE_AN_FBXIMPORTER_HPP
