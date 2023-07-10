//
// Created by aojoie on 4/13/2023.
//

#ifndef OJOIE_TOKEN_HPP
#define OJOIE_TOKEN_HPP

#include <ojoie/Configuration/typedef.h>
#include <string_view>

namespace AN::ShaderLab {

enum TokenKind {
#define TOK(ID) kToken_##ID,
#include <ojoie/ShaderLab/TokenKind.def>
    kTokenNum
};

AN_API const char *GetTokenName(TokenKind Kind);

AN_API bool IsTokenKeyword(TokenKind kind);

class AN_API Token {
    
    TokenKind kind;
    const char *ptr;
    size_t length;

    friend class Lexer;

public:
    Token() : kind(kToken_unknown), ptr(), length() {}

    bool is(TokenKind K) const { return kind == K; }

    bool isNot(TokenKind K) const { return kind != K; }

    bool isOneOf(TokenKind K1, TokenKind K2) const {
        return is(K1) || is(K2);
    }

    template<typename... Ts>
    bool isOneOf(TokenKind K1, TokenKind K2,
                 Ts... Ks) const {
        return is(K1) || isOneOf(K2, Ks...);
    }

    void setKind(TokenKind aKind) {
        Token::kind = aKind;
    }

    TokenKind getKind() const {
        return kind;
    }

    const size_t &getLength() const {
        return length;
    }

    std::string_view getIdentifier() {
        ANAssert(is(kToken_identifier) && "Cannot get identfier of non-identifier");
        return {ptr, length};
    }

    const char *getName() const {
        return GetTokenName(kind);
    }

    std::string_view getRawData() const {
        return {ptr, length};
    }

    std::string_view getStringLiteral() const {
        std::string_view ret(ptr, length);
        ret.remove_suffix(1);
        ret.remove_prefix(1);
        return ret;
    }

    float getFloatLiteral() const;
    int getIntegerLiteral() const;

    bool isKeyword() const {
        return IsTokenKeyword(kind);
    }
};


}

#endif//OJOIE_TOKEN_HPP
