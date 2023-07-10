//
// Created by aojoie on 4/13/2023.
//

#ifndef OJOIE_PARSER_HPP
#define OJOIE_PARSER_HPP

#include <ojoie/ShaderLab/ShaderInfo.hpp>
#include <ojoie/ShaderLab/Lexer.hpp>
#include <ojoie/Core/Exception.hpp>


namespace AN::ShaderLab {

class AN_API Parser {

    bool _hasError;
    Error _error;
    Lexer &lexer;
    Token token;


    void advance() {
        lexer.next(token);
    }

    void error();

    void errorWithMsg(int code, const char *msg);

    bool expect(TokenKind tokenKind);

    bool expectOneOf(TokenKind K1) {
        return expect(K1);
    }

    template<typename... Ts>
    bool expectOneOf(TokenKind K1, Ts... Ks) {
        if (token.isOneOf(K1, Ks...)) {
            return true;
        }
        error();
        return false;
    }

    bool consume(TokenKind tokenKind);


    bool parseShader(ShaderInfo &info);
    bool parseProperties(ShaderInfo &info);
    bool parseSubShader(ShaderInfo &info);
    bool parsePass(SubShader &subShader, std::string &outSource);
    bool parseTags(TagMap &tagMap);
    bool parsePassCommands(ShaderPass &pass);
    bool parseHLSLInclude(std::string &outInclude);
    bool parseHLSLSource(std::string &outSource);

public:

    explicit Parser(Lexer &aLexer) : lexer(aLexer), _hasError() {
        advance();
    }

    const Error &getError() const { return _error; }

    bool hasError() const { return _hasError; }

    ShaderInfo parse();

};



}

#endif//OJOIE_PARSER_HPP
