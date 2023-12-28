//
// Created by Aleudillonam on 8/2/2023.
//

#pragma once

#include <ojoie/Core/Actor.hpp>
#include <ojoie/Core/UUID.hpp>
#include <unordered_map>

namespace AN
{

struct SerializedObjectIdentifier
{
    UUID   uuid;
    UInt64 localID;

    bool operator == (const SerializedObjectIdentifier &) const = default;
};

}

template<>
struct std::hash<AN::SerializedObjectIdentifier>
{
    size_t operator()(const AN::SerializedObjectIdentifier &identifier) const
    {
        return std::hash<std::string>()(identifier.uuid.ToString()) ^ identifier.localID;
    }
};

namespace AN {


typedef bool (*PFN_GetSerializedObjectIdentifierHook)(Object *object, SerializedObjectIdentifier &identifier, void *user);
typedef bool (*PFN_GetSerializedObjectHook)(const SerializedObjectIdentifier &identifier, Object* &object, const char *className, void *user);

class AN_API SerializeManager {

    std::unordered_map<Object *, SerializedObjectIdentifier> m_ObjectToIdentifierMap;
    std::unordered_map<SerializedObjectIdentifier, Object *> m_IdentifierToObjectMap;

    PFN_GetSerializedObjectIdentifierHook m_GetSerializedObjectIdentifierHook;
    PFN_GetSerializedObjectHook           m_GetSerializedObjectHook;
    void *m_User;

public:

    SerializeManager();

    bool SerializeObjectAtPath(Object *object, const char *path);

    bool SerializePrefabAtPath(Actor *actor, const char *path);

    SerializedObjectIdentifier GetSerializedObjectIdentifier(Object *object);

    Object *GetSerializedObject(const SerializedObjectIdentifier &identifier, const char *className);

    void RegisterSerializedObjectIdentifier(Object *object, const SerializedObjectIdentifier &identifier);

    void HookGetSerializedObjectIdentifier(PFN_GetSerializedObjectIdentifierHook hook, void *user)
    {
        m_GetSerializedObjectIdentifierHook = hook;
        m_User = user;
    }

    void HookGetSerializedObject(PFN_GetSerializedObjectHook hook, void *user)
    {
        m_GetSerializedObjectHook = hook;
        m_User = user;
    }
};

AN_API SerializeManager &GetSerializeManager();

}

