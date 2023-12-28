//
// Created by Aleudillonam on 8/2/2023.
//

#include "Serialize/SerializeManager.hpp"
#include "Serialize/SerializedAsset.h"

namespace AN
{

SerializeManager::SerializeManager()
    : m_GetSerializedObjectIdentifierHook()
{
}

bool SerializeManager::SerializeObjectAtPath(Object *object, const char *path)
{
    SerializedAsset serializedAsset;
    serializedAsset.AddObject(object);
    return serializedAsset.SaveAtPath(path);
}

bool SerializeManager::SerializePrefabAtPath(Actor *actor, const char *path)
{
    SerializedAsset serializedAsset;
    serializedAsset.AddObject(actor);
    return serializedAsset.SaveAtPath(path);
}

SerializedObjectIdentifier SerializeManager::GetSerializedObjectIdentifier(Object *object)
{
    if (m_GetSerializedObjectIdentifierHook)
    {
        SerializedObjectIdentifier identifier{};
        if (m_GetSerializedObjectIdentifierHook(object, identifier, m_User))
        {
            return identifier;
        }
    }

    if (m_ObjectToIdentifierMap.contains(object))
    {
        return m_ObjectToIdentifierMap[object];
    }

    return {};
}

Object *SerializeManager::GetSerializedObject(const SerializedObjectIdentifier &identifier, const char *className)
{
    if (m_GetSerializedObjectHook)
    {
        Object *object{};
        if (m_GetSerializedObjectHook(identifier, object, className, m_User))
        {
            return object;
        }
    }

    if (m_IdentifierToObjectMap.contains(identifier))
    {
        return m_IdentifierToObjectMap[identifier];
    }

    Object *object = NewObject(className);
    m_ObjectToIdentifierMap[object] = identifier;
    m_IdentifierToObjectMap[identifier] = object;

    return object;
}

void SerializeManager::RegisterSerializedObjectIdentifier(Object *object, const SerializedObjectIdentifier &identifier)
{
    m_ObjectToIdentifierMap[object] = identifier;
    m_IdentifierToObjectMap[identifier] = object;
}

SerializeManager &GetSerializeManager()
{
    static SerializeManager serializeManager;
    return serializeManager;
}

}// namespace AN