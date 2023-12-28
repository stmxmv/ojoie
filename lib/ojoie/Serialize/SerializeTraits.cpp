//
// Created by aojoie on 12/16/2023.
//

#include "Serialize/SerializeTraits.hpp"
#include "Serialize/SerializeManager.hpp"
#include "Serialize/Coder/YamlDecoder.hpp"
#include "Serialize/Coder/YamlEncoder.hpp"

namespace AN
{

void TransferUUID(const UUID &uuid, YamlEncoder& coder)
{
    std::string str = uuid.ToString();
    coder.transfer(str, "uuid");
}

void TransferUUID(UUID &uuid, YamlDecoder& coder)
{
    std::string str;
    coder.transfer(str, "uuid");
    uuid.Init(str.c_str());
}

void TransferIDPtr(Object* &ptr, YamlEncoder& coder)
{
    coder.AddMetaFlag(kFlowMappingStyle);

    SerializedObjectIdentifier identifier = GetSerializeManager().GetSerializedObjectIdentifier(ptr);
    if (identifier.uuid.IsValid())
    {
        TransferUUID(identifier.uuid, coder);
    }
    else
    {
        // same file object
    }

    coder.transfer(identifier.localID, "localID");

    if (ptr != nullptr)
    {
        std::string className = ptr->getClassName();
        coder.transfer(className, "className");
    }
}

void TransferIDPtr(Object* &ptr, YamlDecoder& coder)
{
    SerializedObjectIdentifier identifier;

    if (coder.HasNode("uuid"))
    {
        TransferUUID(identifier.uuid, coder);
    }

    coder.transfer(identifier.localID, "localID");

    if (coder.HasNode("className"))
    {
        std::string className;
        coder.transfer(className, "className");
        ptr = GetSerializeManager().GetSerializedObject(identifier, className.c_str());
    }
}

template<>
void TransferIDPtr(Object* &ptr, YamlDecoder& coder)
{
    TransferIDPtr(ptr, coder);
}

template<>
void TransferIDPtr(Object* &ptr, YamlEncoder& coder)
{
    TransferIDPtr(ptr, coder);
}

}

