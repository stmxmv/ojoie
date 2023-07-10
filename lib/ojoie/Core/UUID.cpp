// -*- Nitetrichor -*-
//===--------------------------- `target name` ---------------------------------===//
//
// UUID.cpp
//
// Created by Molybdenum on 11/16/21.
// Copyright (c) 2021 Nitetrichor. All rights reserved.
//===----------------------------------------------------------------------===//


#include "Core/UUID.hpp"

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



bool UUID::init() {
#if defined(_WIN32)
    ::UUID uuid;
    HRESULT hr = ::CoCreateGuid(&uuid);

    memcpy(data, &uuid, Size);

    return SUCCEEDED(hr);
#else
    uuid_generate_time(data);
    return true;
#endif
}

std::string UUID::string() const {
#if defined(_WIN32)
    ::UUID uuid;
    memcpy(&uuid, data, Size);

    RPC_CSTR str;
    UuidToStringA(&uuid, &str);

    char ret[StringBufferSize];
    char* signedStr = reinterpret_cast<char*>(str);
    memcpy(ret, signedStr, StringBufferSize);
    transform(ret, ret + StringBufferSize, ret, ::toupper);

#else
    char ret[StringBufferSize];
    uuid_unparse_upper(data, ret);
#endif
    return { ret };
}


}




