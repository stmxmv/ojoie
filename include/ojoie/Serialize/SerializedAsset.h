//
// Created by aojoie on 12/15/2023.
//

#pragma once

#include <ojoie/Object/Object.hpp>
#include <ojoie/Core/UUID.hpp>
#include <ojoie/Serialize/SerializeManager.hpp>

namespace AN
{

struct ObjectMeta
{
    int serializeVersion = 1;
    UUID uuid;
    UInt64 mainObjectLocalID = 0;

    template<typename Coder>
    void transfer(Coder &coder)
    {
        TRANSFER(serializeVersion);

        std::string uuidString;

        if constexpr (Coder::IsEncoding())
        {
            uuidString = uuid.ToString();
        }

        coder.transfer(uuidString, "uuid");

        if constexpr (Coder::IsDecoding())
        {
            uuid.Init(uuidString.c_str());
        }

        TRANSFER(mainObjectLocalID);
    }
};

class SerializedAsset
{
    UUID    m_UUID;
    Object *m_MainObject;
    std::vector<Object *> m_ObjectList;
    std::unordered_map<Object *, UInt64> m_ObjectToLocalIDMap;
    std::unordered_map<UInt64, Object *> m_LocalIDToObjectMap;

    static bool GetSerializedObjectIdentifierHook(Object *object, SerializedObjectIdentifier &identifier, void *user);
    static bool GetSerializedObjectHook(const SerializedObjectIdentifier &identifier, Object* &object, const char *className, void *user);

    void GenerateMetaAtPath(const char *assetPath);

public:

    SerializedAsset();

    Object *GetMainObject();
    const std::vector<Object *> &GetObjectList() const { return m_ObjectList; }

    void AddObject(Object *object);

    bool SaveAtPath(const char *path);
    bool LoadAtPath(const char *path);

};


}
