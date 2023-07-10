//
// Created by aojoie on 4/13/2023.
//

#ifndef OJOIE_LEXER_HPP
#define OJOIE_LEXER_HPP

#include <ojoie/ShaderLab/Token.hpp>
#include <unordered_map>

namespace AN::ShaderLab {

class AN_API KeywordFilter {
    std::unordered_map<std::string_view, TokenKind> HashTable;

    void addKeyword(std::string_view Keyword,
                    TokenKind TokenCode);

public:
    void initializeKeywords();

    TokenKind getKeyword(
            std::string_view Name,
            TokenKind DefaultTokenCode = kToken_unknown) {
        auto Result = HashTable.find(Name);

        if (Result != HashTable.end()) {
            return Result->second;
        }
        return DefaultTokenCode;
    }
};

class AN_API Lexer {
    const char *source;
    const char *curPtr;
    int row, col;
    KeywordFilter keywordFilter;

    bool bHLSLSource;

    void formToken(Token &Result, const char *TokEnd,
                   TokenKind Kind);

    void formIdentifier(Token &result);

    void formNumber(Token &result);

    void formChar(Token &result);

    void formString(Token &result);

    void formHLSLSource(Token &result);

    void ignoreSingleLineComment();

    void ignoreMultiLineComment();

public:
    explicit Lexer(const char *aSource);

    void next(Token &Result);

    int getRow() const { return row; }
    int getCol() const { return col; }
};



}



#endif//OJOIE_LEXER_HPP
