//
// Created by aojoie on 4/14/2023.
//

#include <ojoie/Utility/Log.h>
#include "ShaderLab/Lexer.hpp"

namespace AN::ShaderLab {

void KeywordFilter::addKeyword(std::string_view Keyword, TokenKind TokenCode) {
    HashTable.insert({Keyword, TokenCode});
}
void KeywordFilter::initializeKeywords() {
#define KEYWORD(NAME, FLAGS) \
    addKeyword(#NAME, kToken_kw_##NAME);
#include "ShaderLab/TokenKind.def"
}

namespace charinfo {

inline bool isASCII(char Ch) {
    return static_cast<unsigned char>(Ch) <= 127;
}

inline bool isVerticalWhitespace(char Ch) {
    return isASCII(Ch) && (Ch == '\r' || Ch == '\n');
}

inline bool isHorizontalWhitespace(char Ch) {
    return isASCII(Ch) && (Ch == ' ' || Ch == '\t' ||
                           Ch == '\f' || Ch == '\v');
}

inline bool isWhitespace(char Ch) {
    return isHorizontalWhitespace(Ch) ||
           isVerticalWhitespace(Ch);
}

inline bool isDigit(char Ch) {
    return isASCII(Ch) && Ch >= '0' && Ch <= '9';
}

inline bool isHexDigit(char Ch) {
    return isASCII(Ch) &&
           (isDigit(Ch) || (Ch >= 'A' && Ch <= 'F'));
}

inline bool isIdentifierHead(char Ch) {
    return isASCII(Ch) && (Ch == '_' || (Ch >= 'A' && Ch <= 'Z') || (Ch >= 'a' && Ch <= 'z'));
}

inline bool isIdentifierBody(char Ch) {
    return isIdentifierHead(Ch) || isDigit(Ch);
}

}// namespace charinfo


void Lexer::formToken(Token &Result, const char *TokEnd, TokenKind Kind) {
    size_t TokLen = TokEnd - curPtr;
    Result.ptr    = curPtr;

    Result.length = TokLen;
    Result.kind   = Kind;
    curPtr        = TokEnd;

    col += TokLen;
}

void Lexer::formIdentifier(Token &result) {
    const char *start = curPtr;
    const char *end   = curPtr + 1;
    while (charinfo::isIdentifierBody(*end)) {
        ++end;
    }

    std::string_view const identifier(start, end - start);

    formToken(result, end, keywordFilter.getKeyword(identifier, kToken_identifier));
}
void Lexer::formNumber(Token &result) {
    const char *start = curPtr;
    const char *end   = curPtr + 1;

    bool point = false;

    // check hex start
    if (*start == '0' && *end == 'x') {
        ++end;
    }

    for (;;) {

        if (*end == '.') {
            if (!point) {
                point = true;
            } else {
                /// illegal number report, double point
                AN_LOG(Error, "ShaderLab source illegal number");
            }

        } else if (!charinfo::isDigit(*end)) {
            break;
        }

        ++end;
    }

    /// TODO check number literal suffix

    if (point) {
        formToken(result, end, kToken_float_literal);
    } else {
        formToken(result, end, kToken_integer_literal);
    }
}

void Lexer::formChar(Token &result) {
    const char *start = curPtr;
    const char *end   = curPtr + 1;

    while (*end && (*end != *start || *(end - 1) == '\\') && !charinfo::isVerticalWhitespace(*end)) {
        ++end;
    }

    /// report error on *end is vertical whitespace
    if (charinfo::isVerticalWhitespace(*end)) {
        AN_LOG(Error, "ShaderLab source unterminated char literal");
    }

    /// *end = *start
    formToken(result, end + 1, kToken_char_literal);
}
void Lexer::formString(Token &result) {
    const char *start = curPtr;
    const char *end   = curPtr + 1;

    while (*end && (*end != *start || *(end - 1) == '\\') && !charinfo::isVerticalWhitespace(*end)) {
        ++end;
    }
    /// report error on *end is vertical whitespace
    if (charinfo::isVerticalWhitespace(*end)) {
        AN_LOG(Error, "ShaderLab source unterminated string literal");
    }

    /// *end = *start
    formToken(result, end + 1, kToken_string_literal);
}

void Lexer::ignoreSingleLineComment() {
    const char *end = curPtr + 2;
    while (*end && !charinfo::isVerticalWhitespace(*end)) {
        ++end;
    }
    curPtr = end;
}

void Lexer::ignoreMultiLineComment() {
    const char *end = curPtr + 2;

    while (*end && (*end != '*' || *(end + 1) != '/')) {
        ++end;
    }

    if (*end == '*') {
        curPtr = end + 2;
        return;
    }

    AN_LOG(Error, "ShaderLab source unterminated comment");
    curPtr = end;
}

void Lexer::formHLSLSource(Token &result) {
    const char *end = curPtr + 1;
    while (*end && std::string_view(end, 7) != "ENDHLSL") {
        if (*end == '\n') {
            ++row;
            col = 1;
        } else if (*end == ' ') {
            ++col;
        }
        ++end;
    }
    /// end == "ENDHLSL" or eof
    formToken(result, end, kToken_hlsl_source);
}

void Lexer::next(Token &result) {

start:
    /// skip all kinds of white space
    while (*curPtr && charinfo::isWhitespace(*curPtr)) {
        if (*curPtr == '\n') {
            ++row;
            col = 1;
        } else if (*curPtr == ' ') {
            ++col;
        }
        ++curPtr;
    }

    /// ignore comments
    if (*curPtr == '/') {
        if (*(curPtr + 1) == '/') {
            ignoreSingleLineComment();
            goto start;
        }
        if (*(curPtr + 1) == '*') {
            ignoreMultiLineComment();
            goto start;
        }
    }

    /// check if end of string
    if (!*curPtr) {
        result.setKind(kToken_eof);
        return;
    }

    if (bHLSLSource) {
        formHLSLSource(result);
        bHLSLSource = false;
        return;
    }

    /// common identifier and keyword like 2D, 3D, etc
    if (charinfo::isIdentifierHead(*curPtr) || (charinfo::isDigit(*curPtr) && *(curPtr + 1) == 'D')) {
        formIdentifier(result);

        if (result.isOneOf(kToken_kw_HLSLINCLUDE, kToken_kw_HLSLPROGRAM)) {
            bHLSLSource = true;
        }

        return;
    }
    if (charinfo::isDigit(*curPtr)) {
        formNumber(result);
        return;
    }
    if (*curPtr == '\'') {
        formChar(result);
        return;
    }
    if (*curPtr == '"') {
        formString(result);
        return;
    }

    switch (*curPtr) {
#define CASE(ch, tok)                       \
    case ch:                                \
        formToken(result, curPtr + 1, tok); \
        return

        /// handle single character punctuator
        CASE('{', kToken_left_curly_bracket);
        CASE('}', kToken_right_curly_bracket);
        CASE('[', kToken_left_square_bracket);
        CASE(']', kToken_right_square_bracket);
        CASE('(', kToken_left_bracket);
        CASE(')', kToken_right_bracket);
        CASE(';', kToken_semi);
        CASE('?', kToken_question_mark);
        CASE('~', kToken_bitwise_not);
        CASE(',', kToken_comma);

#undef CASE

        /// handle multi character punctuator
        case '!':
            if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_not_equal);
            } else {
                formToken(result, curPtr + 1, kToken_logical_not);
            }
            return;
        case '#':
            if (*(curPtr + 1) == '#') {
                formToken(result, curPtr + 2, kToken_double_hashtag);
            } else {
                formToken(result, curPtr + 1, kToken_hashtag);
            }
            return;

        case ':':
            if (*(curPtr + 1) == ':') {
                formToken(result, curPtr + 2, kToken_double_colon);
            } else {
                formToken(result, curPtr + 1, kToken_colon);
            }
            return;

        case '.':
            if (*(curPtr + 1) == '.' && *(curPtr + 2) == '.') {
                formToken(result, curPtr + 3, kToken_ellipsis);
            } else if (*(curPtr + 1) == '*') {
                formToken(result, curPtr + 2, kToken_pointer_to_member1);
            } else {
                formToken(result, curPtr + 1, kToken_period);
            }
            return;

        case '+':
            if (*(curPtr + 1) == '+') {
                formToken(result, curPtr + 2, kToken_increment);
            } else if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_plus_equal);
            } else {
                formToken(result, curPtr + 1, kToken_plus);
            }

            return;

        case '-':
            if (*(curPtr + 1) == '-') {
                formToken(result, curPtr + 2, kToken_decrement);
            } else if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_minus_equal);
            } else if (*(curPtr + 1) == '>') {
                if (*(curPtr + 2) == '*') {
                    formToken(result, curPtr + 3, kToken_pointer_to_member2);
                } else {
                    formToken(result, curPtr + 2, kToken_member_access);
                }
            } else {
                formToken(result, curPtr + 1, kToken_minus);
            }
            return;

        case '*':
            if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_mul_equal);
            } else {
                formToken(result, curPtr + 1, kToken_star);
            }
            return;

        case '/':
            if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_division_equal);
            } else {
                formToken(result, curPtr + 1, kToken_division);
            }
            return;

        case '%':
            if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_mod_equal);
            } else {
                formToken(result, curPtr + 1, kToken_mod);
            }
            return;

        case '^':
            if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_xor_equal);
            } else {
                formToken(result, curPtr + 1, kToken_bitwise_xor);
            }
            return;

        case '&':
            if (*(curPtr + 1) == '&') {
                formToken(result, curPtr + 2, kToken_logical_and);
            } else if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_and_equal);
            } else {
                formToken(result, curPtr + 1, kToken_ampersand);
            }
            return;

        case '|':
            if (*(curPtr + 1) == '|') {
                formToken(result, curPtr + 2, kToken_logical_or);
            } else if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_or_equal);
            } else {
                formToken(result, curPtr + 1, kToken_bitwise_or);
            }
            return;

        case '=':
            if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_equal);
            } else {
                formToken(result, curPtr + 1, kToken_assign);
            }
            return;

        case '<':
            if (*(curPtr + 1) == '<') {
                if (*(curPtr + 2) == '=') {
                    formToken(result, curPtr + 3, kToken_left_shift_equal);
                } else {
                    formToken(result, curPtr + 2, kToken_left_shift);
                }
            } else if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_less_equal);
            } else {
                formToken(result, curPtr + 1, kToken_less);
            }
            return;

        case '>':
            if (*(curPtr + 1) == '>') {
                if (*(curPtr + 2) == '=') {
                    formToken(result, curPtr + 3, kToken_right_shift_equal);
                } else {
                    formToken(result, curPtr + 2, kToken_right_shift);
                }
            } else if (*(curPtr + 1) == '=') {
                formToken(result, curPtr + 2, kToken_greater_equal);
            } else {
                formToken(result, curPtr + 1, kToken_greater);
            }
            return;

        default:
            formToken(result, curPtr + 1, kToken_unknown);
            return;
    }
}

Lexer::Lexer(const char *aSource)
    : source(aSource), curPtr(aSource), bHLSLSource(), row(1), col(1) {
    keywordFilter.initializeKeywords();
}


}// namespace AN::ShaderLab