//
// Created by aojoie on 5/6/2023.
//

#include "Utility/String.hpp"
#include "Configuration/typedef.h"

namespace AN {


void BytesToHexString(const void *data, size_t bytes, char *str) {
    static const char kHexToLiteral[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    for (size_t i = 0; i < bytes; i++) {
        UInt8 b        = ((UInt8 *) data)[i];
        str[2 * i + 0] = kHexToLiteral[b >> 4];
        str[2 * i + 1] = kHexToLiteral[b & 0xf];
    }
}

void HexStringToBytes(char *str, size_t bytes, void *data) {
    for (size_t i = 0; i < bytes; i++) {
        UInt8 b;
        char  ch = str[2 * i + 0];
        if (ch <= '9')
            b = (ch - '0') << 4;
        else
            b = (ch - 'a' + 10) << 4;

        ch = str[2 * i + 1];
        if (ch <= '9')
            b |= (ch - '0');
        else
            b |= (ch - 'a' + 10);

        ((UInt8 *) data)[i] = b;
    }
}

}// namespace AN