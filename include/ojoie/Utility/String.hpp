//
// Created by aojoie on 5/6/2023.
//

#ifndef OJOIE_STRING_HPP
#define OJOIE_STRING_HPP

#include <ojoie/Configuration/typedef.h>
#include <string_view>

namespace AN {


AN_API void BytesToHexString(const void *data, size_t bytes, char* str);
AN_API void HexStringToBytes (char* str, size_t numBytes, void *data);

inline bool IsSpace(char c) { return c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == ' '; }

inline char ToLower(char v) {
    if (v >= 'A' && v <= 'Z') {
        return static_cast<char>(v | 0x20);
    }
    return v;
}

inline char ToUpper(char v) {
    if (v >= 'a' && v <= 'z') {
        return static_cast<char>(v & 0xdf);
    }
    return v;
}

}



#endif//OJOIE_STRING_HPP
