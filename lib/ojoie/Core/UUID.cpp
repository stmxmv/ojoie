// -*- Nitetrichor -*-
//===--------------------------- `target name` ---------------------------------===//
//
// UUID.cpp
//
// Created by Molybdenum on 11/16/21.
// Copyright (c) 2021 Nitetrichor. All rights reserved.
//===----------------------------------------------------------------------===//


#include "Core/UUID.hpp"
#include "Utility/String.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <objbase.h>
#include <string>
#include <algorithm>
#include <cctype>
#else
#include <uuid/uuid.h>
#endif
using namespace std;
namespace AN {



bool UUID::Init() {
#if defined(_WIN32)
    ::UUID uuid;
    HRESULT hr = ::CoCreateGuid(&uuid);

    memcpy(data, &uuid, Size);

    return SUCCEEDED(hr) && IsValid();
#else
    uuid_generate_time(data);
    return true;
#endif
}

bool UUID::Init(const char *str)
{
    if (strlen(str) < StringSize)
    {
        return false;
    }
    HexStringToBytes(str, sizeof(data), &data);
    return IsValid();
}

std::string UUID::ToString() const {
    std::string str(sizeof(data) * 2, 0);
    BytesToHexString(&data, sizeof(data), str.data());
    return str;
}


}




