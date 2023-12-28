//
// Created by aojoie on 12/13/2023.
//

#pragma once

#include <ojoie/Serialize/SerializeTraits.hpp>
#include <ojoie/Utility/Log.h>

namespace AN
{

template<typename T, typename = void>
struct IsIDPtr
{
    constexpr static bool value = false;
};

template<typename T>
struct IsIDPtr<T, std::enable_if_t<std::is_pointer_v<T> && std::is_base_of_v<Object, std::remove_pointer_t<T>>>>
{
    constexpr static bool value = true;
};

template<typename T>
class IDPtrRemapper
{
    bool m_IsEncoding;

    template<typename T>
    void code(T& data, const char* name, int metaFlags = 0)
    {
    }

public:

    constexpr static bool IsEncoding() { return true; }
    constexpr static bool IsDecoding() { return false; }


    template<typename T>
    void transfer(T& data, const char* name, int metaFlags = 0)
    {
        if constexpr (SerializeTraits<T>::MightContainIDPtr())
        {
            if constexpr (IsIDPtr<T>::value)
            {
                if (m_IsEncoding)
                {
                    AN_LOG(Debug, "Found IDPtr 0x%p", data);
                }
                else
                {

                }
            }
        }
    }

    void PushMetaFlag (int flag) {}
    void PopMetaFlag () {}
    void AddMetaFlag(int mask) {}

    void transferTypeless(size_t &value, const char* name, int metaFlag = 0) {}
    void transferTypelessData(void* data, size_t size, int metaFlags = 0) {}
};

}

