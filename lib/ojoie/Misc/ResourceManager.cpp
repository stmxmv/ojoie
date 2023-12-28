//
// Created by aojoie on 6/18/2023.
//

#include "Misc/ResourceManager.hpp"
#include "Object/Class.hpp"
#include "HAL/File.hpp"

#include "Serialize/SerializedAsset.h"
#include "Serialize/SerializeManager.hpp"

#include "Render/Material.hpp"

#include <ojoie/Serialize/Coder/YamlDecoder.hpp>
#include <ojoie/IO/FileInputStream.hpp>
#include <ojoie/Utility/Path.hpp>

#include <filesystem>

namespace AN {

static UInt64 GetResourceID()
{
    static int id = 1;
    return id++;
}

void ResourceManager::loadBuiltinResources() {

#define LOAD_BUILTIN_RESOURCE(cls, name) \
    do {                                 \
         Object *object;                 \
         object = loadResource(cls, name, GetApplicationFolder().c_str()); \
         ANAssert(object != nullptr);    \
         GetSerializeManager().RegisterSerializedObjectIdentifier(object, { {}, GetResourceID() }); \
    } while(0)

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

    Material *defaultMat = NewObject<Material>();
    defaultMat->init((Shader *)GetResourceManager().getResource(Shader::GetClassNameStatic(), "Default"), "Default");
    RegisterResource(defaultMat, "Default");
}

void ResourceManager::RegisterResource(Object *object, const char *name)
{
    std::string key = std::string(object->getClassName()) + "_" + std::string(name);
    resourceMap.insert({ key, object });
    GetSerializeManager().RegisterSerializedObjectIdentifier(object, { {}, GetResourceID() });
}

Object *ResourceManager::getResourceAtPath(const char *path) {
    std::string convertedPath = ConvertPath(path);
    if (resourcePathMap.contains(convertedPath)) {
        return resourcePathMap[convertedPath];
    }
    return nullptr;
}

void ResourceManager::unloadResource(Object *asset)
{
    if (asset)
    {
        for (auto &&[k, v] : resourcePathMap)
        {
            if (v == asset)
            {
                resourcePathMap.erase(k);
                break;
            }
        }

        for (auto &&[k, v] : resourceMap)
        {
            if (v == asset)
            {
                resourceMap.erase(k);
                break;
            }
        }
        DestroyObject(asset);
    }
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
    resourcePathMap[convertedPath] = object;
}

Object *ResourceManager::loadResourceAtPath(const char *_path) {

    Path path(_path);

    SerializedAsset serializedAsset;
    if (!serializedAsset.LoadAtPath(_path))
    {
        return nullptr;
    }

    Object *mainObject = serializedAsset.GetMainObject();
    std::string className = mainObject->getClassName();

    for (Object *object : serializedAsset.GetObjectList())
    {
        if (!object->initAfterDecode()) {
            DestroyObject(object);
            return nullptr;
        }

        if (object->isDerivedFrom<Component>())
        {
            Component *component = (Component *)object;
            Message message;
            message.sender = this;
            message.data = (intptr_t)object;
            message.name = kDidAddComponentMessage;
            component->getActor().sendMessage(message);

            if (!component->getActor().isActive()) {
                component->deactivate();
            }
        }
    }


    std::string key = std::string(className) + "_" + std::string(path.GetLastComponentWithoutExtension());

    /// if exist, destroy and replace it
    if (auto it = resourceMap.find(key); it != resourceMap.end()) {
        DestroyObject(it->second);
        resourceMap.erase(it);
    }

    /// insert in map
    resourceMap.insert({ key, mainObject });
    resourcePathMap[ConvertPath(_path)] = mainObject;
    return mainObject;
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
            if (!file.Open(filePath.c_str(), kFilePermissionRead)) continue;

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
    if (!file.Open(filePath.c_str(), kFilePermissionRead)) return nullptr;
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