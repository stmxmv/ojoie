//
// Created by aojoie on 6/18/2023.
//

#include "Misc/ResourceManager.hpp"
#include "Object/Class.hpp"
#include "HAL/File.hpp"
#include <ojoie/Serialize/Coder/YamlDecoder.hpp>
#include <ojoie/IO/FileInputStream.hpp>
#include <ojoie/Utility/Path.hpp>

#include <filesystem>

namespace AN {

void ResourceManager::loadBuiltinResources() {

#define LOAD_BUILTIN_RESOURCE(cls, name) ANAssert(loadResource(cls, name, GetApplicationFolder().c_str()) != nullptr)

    LOAD_BUILTIN_RESOURCE("Shader", "BlitCopy");
    LOAD_BUILTIN_RESOURCE("Shader", "GUIBlit");
    LOAD_BUILTIN_RESOURCE("Shader", "Default");
    LOAD_BUILTIN_RESOURCE("Shader", "IMGUI");
    LOAD_BUILTIN_RESOURCE("Shader", "Skybox");
    LOAD_BUILTIN_RESOURCE("Shader", "Skybox-Procedural");
    LOAD_BUILTIN_RESOURCE("Shader", "test");
    LOAD_BUILTIN_RESOURCE("Shader", "SceneViewSelected");
    LOAD_BUILTIN_RESOURCE("Shader", "SceneViewSelection");
    LOAD_BUILTIN_RESOURCE("Texture2D", "FileIconTex");
    LOAD_BUILTIN_RESOURCE("Texture2D", "FolderIconTex");
    LOAD_BUILTIN_RESOURCE("Texture2D", "FolderEmptyIconTex");
}

Object *ResourceManager::getResourceExact(const char *path) {
    std::string convertedPath = ConvertPath(path);
    if (resourcePathMap.contains(convertedPath)) {
        return resourcePathMap[convertedPath];
    }
    return nullptr;
}

void ResourceManager::resetResourcePath(Object *object, const char *pathIn) {
    std::string convertedPath = ConvertPath(pathIn);
    for (auto &[path, obj] : resourcePathMap) {
        if (obj == object) {
            resourcePathMap.erase(path);
            resourcePathMap[convertedPath] = object;
            return;
        }
    }
}

Object *ResourceManager::loadResourceExact(const char *_path) {
    std::filesystem::path path(_path);
    if (!exists(path)) return nullptr;

    std::string className;

    /// try get the resource class name
    {
        File file;
        if (!file.open(path.string().c_str(), kFilePermissionRead)) return nullptr;

        FileInputStream fileInputStream(file);
        YamlDecoder     decoder(fileInputStream);
        YAMLNode       *node = decoder.getCurrentNode();
        if (YAMLMapping *map= dynamic_cast<YAMLMapping *>(node); map) {
            map = dynamic_cast<YAMLMapping *>(map->begin()->second);
            if (map) {
                YAMLScalar *scalar = dynamic_cast<YAMLScalar *>(map->get("classname"));
                if (scalar) {
                    className = scalar->getStringValue();
                }
            }
        }
        node->release();
    }

    if (className.empty()) {
        return nullptr;
    }

    File file;
    if (!file.open(path.string().c_str(), kFilePermissionRead)) return nullptr;
    FileInputStream fileInputStream(file);
    YamlDecoder     decoder(fileInputStream);

    Object *object = NewObject(className);
    object->redirectTransferVirtual(decoder);

    if (!object->initAfterDecode()) {
        DestroyObject(object);
        return nullptr;
    }

    std::string key = std::string(className) + "_" + path.stem().string();

    /// if exist, destroy and replace it
    if (auto it = resourceMap.find(key); it != resourceMap.end()) {
        DestroyObject(it->second);
        resourceMap.erase(it);
    }

    /// insert in map
    resourceMap.insert({ key, object });
    resourcePathMap[ConvertPath(_path)] = object;
    return object;
}

Object *ResourceManager::loadResource(const char *className, const char *name, const char *searchPath) {

    std::string key = std::string(className) + "_" + std::string(name);

    if (auto it = resourceMap.find(key); it != resourceMap.end()) {
        DestroyObject(it->second);
        resourceMap.erase(it);
    }

    std::filesystem::path rootDir;
    if (searchPath) {
        rootDir = searchPath;
    } else {
        rootDir = GetCurrentDirectory() + "/Data/Assets";
    }

    std::string filePath;

    if (!exists(rootDir)) {
        return nullptr;
    }

    for (const auto &entry : std::filesystem::recursive_directory_iterator{ rootDir }) {
        if (entry.is_directory()) continue;
        if (entry.path().stem() == name){
            filePath = entry.path().string();

            File file;
            if (!file.open(filePath.c_str(), kFilePermissionRead)) continue;

            FileInputStream fileInputStream(file);
            YamlDecoder     decoder(fileInputStream);
            YAMLNode       *node = decoder.getCurrentNode();

            if (YAMLMapping *map= dynamic_cast<YAMLMapping *>(node); map) {
                map = dynamic_cast<YAMLMapping *>(map->begin()->second);
                if (map) {
                    YAMLScalar *scalar = dynamic_cast<YAMLScalar *>(map->get("classname"));
                    if (scalar && scalar->getStringValue() == className) {
                        node->release();
                        break;
                    }
                }
            }
            node->release();
        }
    }

    if (filePath.empty()) return nullptr;

    File file;
    if (!file.open(filePath.c_str(), kFilePermissionRead)) return nullptr;
    FileInputStream fileInputStream(file);
    YamlDecoder     decoder(fileInputStream);

    Object *object = NewObject(className);
    object->redirectTransferVirtual(decoder);

    if (!object->initAfterDecode()) {
        DestroyObject(object);
        return nullptr;
    }

    /// insert in map
    resourceMap.insert({ key, object });

    return object;
}

Object *ResourceManager::getResource(const char *className, const char *name) {

    std::string key = std::string(className) + "_" + std::string(name);

    if (auto it = resourceMap.find(key); it != resourceMap.end()) {
        return it->second;
    }

    return nullptr;
}

ResourceManager &GetResourceManager() {
    static ResourceManager resourceManager;
    return resourceManager;
}


}