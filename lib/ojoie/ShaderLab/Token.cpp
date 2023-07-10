//
// Created by aojoie on 4/14/2023.
//

#include <ojoie/Utility/Log.h>
#include "ShaderLab/Token.hpp"
#include <charconv>

namespace AN::ShaderLab {

static const char *const TokNames[] = {
#define TOK(ID)           #ID,
#define KEYWORD(ID, FLAG) #ID,
#include "ShaderLab/TokenKind.def"
#undef KEYWORD
        nullptr
};

const char *GetTokenName(TokenKind Kind) {
    if (Kind < kTokenNum) {
        return TokNames[(size_t) Kind];
    }
    AN_LOG(Error, "Invalid TokenKind enum value %d", (int)Kind);
    return nullptr;
}

bool IsTokenKeyword(TokenKind kind) {
    switch (kind) {
#define KEYWORD(NAME, FLAGS) \
    case kToken_kw_##NAME:     \
        return true;
#include "ShaderLab/TokenKind.def"
#undef KEYWORD
        default:
            break;
    }
    return false;
}

float Token::getFloatLiteral() const {
    float ret = 0.f;
    auto result = std::from_chars(ptr, ptr + length, ret);
    ANAssert(result.ec != std::errc::invalid_argument);
    return ret;
}

int Token::getIntegerLiteral() const {
    int ret = 0;
    auto result = std::from_chars(ptr, ptr + length, ret);
    ANAssert(result.ec != std::errc::invalid_argument);
    return ret;
}

}