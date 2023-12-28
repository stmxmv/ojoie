//
// Created by aojoie on 12/16/2023.
//
#include "Core/Actor.hpp"
#include "Serialize/SerializedAsset.h"
#include "Utility/Path.hpp"
#include "IO/FileInputStream.hpp"

#include <ojoie/IO/FileOutputStream.hpp>
#include <ojoie/Serialize/Coder/YamlEncoder.hpp>

#include <format>
#include <unordered_map>

namespace AN
{

bool SerializedAsset::GetSerializedObjectIdentifierHook(Object *object, SerializedObjectIdentifier &identifier, void *user)
{
    SerializedAsset *self = (SerializedAsset *)user;
    if (self->m_ObjectToLocalIDMap.contains(object))
    {
        identifier.uuid = {};
        identifier.localID = self->m_ObjectToLocalIDMap[object];
        return true;
    }
    return false;
}

bool SerializedAsset::GetSerializedObjectHook(const SerializedObjectIdentifier &identifier, Object *&object, const char *className, void *user)
{
    SerializedAsset *self = (SerializedAsset *)user;
    if (!identifier.uuid.IsValid() && self->m_LocalIDToObjectMap.contains(identifier.localID))
    {
        object = self->m_LocalIDToObjectMap[identifier.localID];
        return true;
    }

    return false;
}

SerializedAsset::SerializedAsset()
    : m_MainObject()
{
}

Object *SerializedAsset::GetMainObject()
{
    return m_MainObject;
}

void SerializedAsset::AddObject(Object *object)
{
    if (m_MainObject == nullptr)
    {
        m_MainObject = object;
        m_ObjectList.clear(); // make sure main object is the first object
    }

    m_ObjectList.push_back(object);

    if (object->is<Actor>())
    {
        // actor may serialize all child actors and all components
        Actor *actor = (Actor *)object;

        for (Component *component : actor->getComponents())
        {
            AddObject(component);
        }

        for (Transform *childTransform : actor->getTransform()->getChildren())
        {
            AddObject(childTransform->getActorPtr());
        }
    }
}

void SerializedAsset::GenerateMetaAtPath(const char *assetPath)
{
    Path metaPath(assetPath);
    metaPath.Append(".meta");

    ANAssert(m_UUID.Init());

    ObjectMeta objectMeta;
    objectMeta.uuid = m_UUID;
    objectMeta.mainObjectLocalID = m_ObjectToLocalIDMap[GetMainObject()];

    {
        YamlEncoder yamlEncoder;
        File        file;
        file.Open(metaPath.ToString().c_str(), AN::kFilePermissionWrite);
        FileOutputStream fileOutputStream(file);

        objectMeta.transfer(yamlEncoder);

        yamlEncoder.outputToStream(fileOutputStream);
    }
}

bool SerializedAsset::SaveAtPath(const char *path)
{
    if (m_MainObject == nullptr || m_ObjectList.size() == 0)
    {
        return false;
    }

    File assetFile;

    if (!assetFile.Open(path, kFilePermissionWrite))
    {
        return false;
    }

    std::unordered_map<int, int> localIDGen;
    for (Object *object : m_ObjectList)
    {
        int classID = object->getClassID();
        int id = localIDGen[classID]++;
        UInt64 localID = classID * 1000000 + id;

        m_ObjectToLocalIDMap[object] = localID;
        m_LocalIDToObjectMap[localID] = object;
    }

    GenerateMetaAtPath(path);

    for (Object *object : m_ObjectList)
    {
        UInt64 localID = m_ObjectToLocalIDMap[object];
        SerializedObjectIdentifier identifier;
        identifier.uuid = m_UUID;
        identifier.localID = localID;
        GetSerializeManager().RegisterSerializedObjectIdentifier(object, identifier);
    }

    GetSerializeManager().HookGetSerializedObjectIdentifier(GetSerializedObjectIdentifierHook, this);

    assetFile.WriteLine("%YAML 1.1");
    assetFile.WriteLine("%TAG !AN! tag:an.com,2023:");

    FileOutputStream fileOutputStream(assetFile);


    for (Object *object : m_ObjectList)
    {
        UInt64 localID = m_ObjectToLocalIDMap[object];

        int sz = std::snprintf(nullptr, 0, "--- !AN!%s &%llu", object->getClassName(),localID);
        char *buffer = (char *)alloca(sz + 1);
        std::sprintf(buffer, "--- !AN!%s &%llu", object->getClassName(),localID);
        buffer[sz] = 0;

        assetFile.WriteLine(buffer);

        YamlEncoder yamlEncoder;
        object->redirectTransferVirtual(yamlEncoder);
        yamlEncoder.outputToStream(fileOutputStream);
    }

    assetFile.Close();

    GetSerializeManager().HookGetSerializedObjectIdentifier(nullptr, nullptr);

    return true;
}

bool SerializedAsset::LoadAtPath(const char *path)
{
    m_ObjectList.clear();

    Path metaPath(path);
    metaPath.Append(".meta");

    ObjectMeta objectMeta;
    if (!metaPath.Exists())
    {
        // meta not found, generate meta
        AN_LOG(Info, "%s path meta not found, generating", path);
        GenerateMetaAtPath(path);
    }
    else
    {

        File        file;
        if (!file.Open(metaPath.ToString().c_str(), AN::kFilePermissionRead))
        {
            return false;
        }
        FileInputStream fileInputStream(file);
        YamlDecoder decoder(fileInputStream);
        objectMeta.transfer(decoder);

        m_UUID = objectMeta.uuid;
    }

    File assetFile;

    if (!assetFile.Open(path, kFilePermissionRead))
    {
        return false;
    }

//    assetFile.WriteLine("%YAML 1.1");
//    assetFile.WriteLine("%TAG !AN! tag:an.com,2023:");
    assetFile.ReadLine();
    assetFile.ReadLine();

    std::string info;
    std::string buffer;

    info = assetFile.ReadLine();

    while (!assetFile.IsEOF())
    {
        if (assetFile.IsEOF() || info.empty())
        {
            break;
        }

        // limit className to 128 characters
        char   className[128];
        UInt64 localID = 0;
        int    n       = sscanf(info.c_str(), "--- !AN!%120s &%llu", className, &localID);

        if (n != 2)
        {
            AN_LOG(Error, "Read asset file failed, file may be broken");
            return false;
        }

        SerializedObjectIdentifier identifier{ m_UUID, localID };
        Object *object = GetSerializeManager().GetSerializedObject(identifier, className);
        m_ObjectToLocalIDMap[object] = localID;
        m_LocalIDToObjectMap[localID] = object;

        if (m_MainObject == nullptr && objectMeta.mainObjectLocalID == localID)
        {
            m_MainObject = object;
        }

        m_ObjectList.push_back(object);

        do
        {
            info = assetFile.ReadLine();

            if (assetFile.IsEOF() || info.empty() || info.find("--- !AN!") != std::string::npos)
            {
                break;
            }
        }  while (true);
    }

    GetSerializeManager().HookGetSerializedObject(GetSerializedObjectHook, this);

    int objectIndex = 0;
    assetFile.SetPosition(0);
    assetFile.ReadLine();
    assetFile.ReadLine();
    info = assetFile.ReadLine();

    while (!assetFile.IsEOF())
    {
        if (assetFile.IsEOF() || info.empty())
        {
            break;
        }

        do
        {
            info = assetFile.ReadLine();

            if (assetFile.IsEOF() || info.empty() || info.find("--- !AN!") != std::string::npos)
            {
                break;
            }

            buffer.append(info);
            buffer.append("\n");
        }  while (true);

        YamlDecoder     decoder(buffer.data(), buffer.size());
        m_ObjectList[objectIndex++]->redirectTransferVirtual(decoder);
        buffer.clear();
    }

    GetSerializeManager().HookGetSerializedObject(nullptr, nullptr);

    return true;
}

}