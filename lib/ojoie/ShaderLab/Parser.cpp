//
// Created by aojoie on 4/14/2023.
//

#include <ojoie/Utility/Log.h>
#include "ShaderLab/Parser.hpp"

#include <variant>

namespace AN::ShaderLab {

#define CHECK(x) do { if (!(x)) { _hasError = true; goto __error; } } while(0)

static BlendFactor tokenToBlendFactor(TokenKind kind) {
    switch (kind) {
        case kToken_kw_One:
            return kBlendFactorOne;
        case kToken_kw_Zero:
            return kBlendFactorZero;
        case kToken_kw_SrcColor:
            return kBlendFactorSourceColor;
        case kToken_kw_SrcAlpha:
            return kBlendFactorSourceAlpha;
        case kToken_kw_SrcAlphaSaturate:
            return kBlendFactorSourceAlphaSaturated;
        case kToken_kw_DstColor:
            return kBlendFactorDestinationColor;
        case kToken_kw_DstAlpha:
            return kBlendFactorDestinationAlpha;
        case kToken_kw_OneMinusSrcColor:
            return kBlendFactorOneMinusSourceColor;
        case kToken_kw_OneMinusSrcAlpha:
            return kBlendFactorOneMinusSourceAlpha;
        case kToken_kw_OneMinusDstColor:
            return kBlendFactorOneMinusDestinationColor;
        case kToken_kw_OneMinusDstAlpha:
            return kBlendFactorOneMinusDestinationAlpha;
        default:
            AN_LOG(Error, "invalid argument pass to %s", __func__);
            return kBlendFactorSourceAlpha;
    }
}

static BlendOperation tokenToBlendOperation(TokenKind kind) {
    switch (kind) {
        default:
            AN_LOG(Error, "invalid argument pass to %s", __func__);
        case kToken_kw_Add:
            return kBlendOperationAdd;
        case kToken_kw_Sub:
            return kBlendOperationSubtract;
        case kToken_kw_RevSub:
            return kBlendOperationReverseSubtract;
        case kToken_kw_Min:
            return kBlendOperationMin;
        case kToken_kw_Max:
            return kBlendOperationMax;
    }
}

static CompareFunction tokenToCompareFunction(TokenKind kind) {
    switch (kind) {
        default:
            AN_LOG(Error, "invalid argument pass to %s", __func__);
        case kToken_kw_LEqual:
            return kCompareFunctionLEqual;
        case kToken_kw_Less:
            return kCompareFunctionLess;
        case kToken_kw_Equal:
            return kCompareFunctionEqual;
        case kToken_kw_GEqual:
            return kCompareFunctionGEqual;
        case kToken_kw_Greater:
            return kCompareFunctionGreater;
        case kToken_kw_NotEqual:
            return kCompareFunctionNotEqual;
        case kToken_kw_Always:
            return kCompareFunctionAlways;
    }
}

void Parser::error() {
    _error = Error(0, std::format("Unexpected token at {}:{} type: {} value: {}",
                                  lexer.getRow(), lexer.getCol(), (int)token.getKind(), token.getRawData()));
//    AN_LOG(Error, "%s", std::format("Unexpected token type: {} value: {}", (int)token.getKind(), token.getRawData()).c_str());
    _hasError = true;
}

void Parser::errorWithMsg(int code, const char *msg) {
    _error = Error(code, msg);
    _hasError = true;
}

bool Parser::expect(TokenKind tokenKind) {
    if (token.getKind() != tokenKind) {
        error();
        return false;
    }
    return true;
}

bool Parser::consume(TokenKind tokenKind) {
    if (!expect(tokenKind)) {
        return false;
    }
    advance();
    return true;
}

bool Parser::parseShader(ShaderInfo &info) {
    CHECK(consume(kToken_kw_Shader));

    CHECK(expect(kToken_string_literal));
    info.name = token.getStringLiteral();
    advance();

    CHECK(consume(kToken_left_curly_bracket));

    if (token.is(kToken_kw_Properties)) {
        CHECK(parseProperties(info));
    }

    while (token.is(kToken_kw_SubShader)) {
        CHECK(parseSubShader(info));
    }

    CHECK(consume(kToken_right_curly_bracket));

    return true;

__error:
    return false;
}

bool Parser::parseProperties(ShaderInfo &info) {

    CHECK(consume(kToken_kw_Properties));
    CHECK(consume(kToken_left_curly_bracket));

    while (token.is(kToken_identifier)) {
        Property aProperty{};
        aProperty.name = token.getIdentifier();
        advance();

        CHECK(consume(kToken_left_bracket));

        CHECK(expect(kToken_string_literal));
        aProperty.description = token.getStringLiteral();
        advance();

        CHECK(consume(kToken_comma));

        CHECK(token.isKeyword());

        switch (token.getKind()) {
            case kToken_kw_Cube:
            case kToken_kw_2D:
                aProperty.type = kShaderPropertyTexture;
                aProperty.dimension = 2;
                break;
            case kToken_kw_Color:
                aProperty.color = true;
            case kToken_kw_Vector:
                aProperty.type = kShaderPropertyFloat;
                aProperty.dimension = 4;
                break;

            case kToken_kw_Float:
                aProperty.type = kShaderPropertyFloat;
                aProperty.dimension = 1;
                aProperty.range.float_min = std::numeric_limits<float>::lowest();
                aProperty.range.float_max = std::numeric_limits<float>::max();
                break;

            case kToken_kw_Integer:
                aProperty.type = kShaderPropertyInt;
                aProperty.dimension = 1;
                aProperty.range.int_min = std::numeric_limits<int>::min();
                aProperty.range.int_max = std::numeric_limits<int>::max();
                break;

            case kToken_kw_Range: {
                advance();
                CHECK(consume(kToken_left_bracket));
                std::variant<int, float> range_min, range_max;

                CHECK(expectOneOf(kToken_integer_literal, kToken_float_literal));
                if (token.is(kToken_integer_literal)) {
                    range_min = token.getIntegerLiteral();
                } else if (token.is(kToken_float_literal)) {
                    range_min = token.getFloatLiteral();
                }
                advance();
                CHECK(consume(kToken_comma));

                CHECK(expectOneOf(kToken_integer_literal, kToken_float_literal));
                if (token.is(kToken_integer_literal)) {
                    range_max = token.getIntegerLiteral();
                } else if (token.is(kToken_float_literal)) {
                    range_max = token.getFloatLiteral();
                }


//                if (std::holds_alternative<int>(range_min) && std::holds_alternative<int>(range_max)) {
//                    aProperty.type = kShaderPropertyInt;
//                } else {
//                    aProperty.type = kShaderPropertyFloat;
//                }

                /// range type must be float
                aProperty.type = kShaderPropertyFloat;
                aProperty.dimension = 1;


                std::visit([&aProperty](auto val) {
                    aProperty.range.float_min = (float)val;
                }, range_min);

                std::visit([&aProperty](auto val) {
                    aProperty.range.float_max = (float)val;
                }, range_max);

                advance();

            }
                break;

            default:
                goto __error;
        }

        advance();

        CHECK(consume(kToken_right_bracket));
        CHECK(consume(kToken_assign));

        const auto getFloatLiteralConvert = [this]() -> float {
            if (token.is(kToken_integer_literal)) {
                return (float) token.getIntegerLiteral();
            }
            return token.getFloatLiteral();
        };



        switch (aProperty.type) {
            case kShaderPropertyFloat:
                if (aProperty.dimension == 1) {
                    CHECK(expectOneOf(kToken_integer_literal, kToken_float_literal));
                    aProperty.defaultValue.floatValue = getFloatLiteralConvert();
                    advance();

                } else {
                    /// Vector4f
                    ANAssert(aProperty.dimension == 4);
                    CHECK(consume(kToken_left_bracket));

                    CHECK(expectOneOf(kToken_integer_literal, kToken_float_literal));
                    aProperty.defaultValue.vector4f.x = getFloatLiteralConvert();
                    advance();

                    CHECK(consume(kToken_comma));

                    CHECK(expectOneOf(kToken_integer_literal, kToken_float_literal));
                    aProperty.defaultValue.vector4f.y = getFloatLiteralConvert();
                    advance();

                    CHECK(consume(kToken_comma));

                    CHECK(expectOneOf(kToken_integer_literal, kToken_float_literal));
                    aProperty.defaultValue.vector4f.z = getFloatLiteralConvert();
                    advance();

                    CHECK(consume(kToken_comma));

                    CHECK(expectOneOf(kToken_integer_literal, kToken_float_literal));
                    aProperty.defaultValue.vector4f.w = getFloatLiteralConvert();
                    advance();

                    CHECK(consume(kToken_right_bracket));
                }

                break;
            case kShaderPropertyInt:
                /// dimension must be 1
                ANAssert(aProperty.dimension == 1);
                CHECK(expect(kToken_integer_literal));
                aProperty.defaultValue.intValue = token.getIntegerLiteral();
                advance();
                break;

            case kShaderPropertyTexture: {
                CHECK(expect(kToken_string_literal));
                Name name(token.getStringLiteral());
                aProperty.defaultStringValue = name.c_str();
                advance();

                CHECK(consume(kToken_left_curly_bracket));
                CHECK(consume(kToken_right_curly_bracket));
            }
                break;
            default:
                goto __error;
        }

        info.properties.push_back(aProperty);
    }

    CHECK(consume(kToken_right_curly_bracket));

    return true;
__error:
    return false;
}

bool Parser::parseSubShader(ShaderInfo &info) {
    SubShader subShader{};
    info.subShaderHLSLIncludes.emplace_back();
    info.passHLSLSources.emplace_back();

    CHECK(consume(kToken_kw_SubShader));
    CHECK(consume(kToken_left_curly_bracket));


    if (token.is(kToken_kw_Tags)) {
        CHECK(parseTags(subShader.tagMap));
    }

    if (token.is(kToken_kw_HLSLINCLUDE)) {
        CHECK(parseHLSLInclude(info.subShaderHLSLIncludes.back()));
    }

    while (token.is(kToken_kw_Pass)) {
        info.passHLSLSources.back().emplace_back();
        CHECK(parsePass(subShader, info.passHLSLSources.back().back()));
    }

    CHECK(consume(kToken_right_curly_bracket));

    info.subShaders.push_back(subShader);
    return true;
__error:
    return false;
}

bool Parser::parsePass(SubShader &subShader, std::string &outSource) {
    ShaderPass pass{};
    /// setting default values
    pass.ZWrite = true;
    pass.ZTestOperation = kCompareFunctionLEqual;
    pass.blending = false;
    pass.colorWriteMask = kColorWriteMaskAll;
    pass.cullMode = kCullModeBack;
    pass.ZClip = true;


    CHECK(consume(kToken_kw_Pass));
    CHECK(consume(kToken_left_curly_bracket));

    if (token.is(kToken_kw_Tags)) {
        parseTags(pass.tagMap);
    }

    CHECK(parsePassCommands(pass));

    CHECK(expect(kToken_kw_HLSLPROGRAM));
    CHECK(parseHLSLSource(outSource));

    CHECK(consume(kToken_right_curly_bracket));

    subShader.passes.push_back(pass);

    return true;
__error:
    return false;
}

bool Parser::parseTags(TagMap &tagMap) {
    CHECK(consume(kToken_kw_Tags));
    CHECK(consume(kToken_left_curly_bracket));

    while (token.is(kToken_string_literal)) {
        Name key(token.getStringLiteral());
        advance();

        CHECK(consume(kToken_assign));

        CHECK(expect(kToken_string_literal));
        Name value(token.getStringLiteral());
        advance();

        tagMap[key] = value;
    }

    CHECK(consume(kToken_right_curly_bracket));
    return true;
__error:
    return false;
}

bool Parser::parsePassCommands(ShaderPass &pass) {
    for (;;) {
        switch (token.getKind()) {
            case kToken_kw_Cull:
                advance();
                CHECK(expectOneOf(kToken_kw_Back, kToken_kw_Front, kToken_kw_Off));
                switch (token.getKind()) {
                    case kToken_kw_Back:
                        pass.cullMode = kCullModeBack;
                        break;
                    case kToken_kw_Front:
                        pass.cullMode = kCullModeFront;
                        break;
                    case kToken_kw_Off:
                        pass.cullMode = kCullModeNone;
                        break;
                    default:
                        /// never reach this line
                        goto __error;
                }
                advance();
                break;

            case kToken_kw_Blend:
                advance();
                CHECK(expectOneOf(kToken_kw_Off, kToken_kw_One, kToken_kw_Zero, kToken_kw_SrcColor,
                                  kToken_kw_SrcAlpha, kToken_kw_SrcAlphaSaturate, kToken_kw_DstColor,
                                  kToken_kw_DstAlpha, kToken_kw_OneMinusSrcColor, kToken_kw_OneMinusSrcAlpha,
                                  kToken_kw_OneMinusDstColor, kToken_kw_OneMinusDstAlpha));
                if (token.is(kToken_kw_Off)) {
                    pass.blending = false;
                    break;
                }

                pass.blending = true;
                pass.sourceBlendFactor = tokenToBlendFactor(token.getKind());
                advance();

                CHECK(expectOneOf(kToken_kw_Off, kToken_kw_One, kToken_kw_Zero, kToken_kw_SrcColor,
                                  kToken_kw_SrcAlpha, kToken_kw_SrcAlphaSaturate, kToken_kw_DstColor,
                                  kToken_kw_DstAlpha, kToken_kw_OneMinusSrcColor, kToken_kw_OneMinusSrcAlpha,
                                  kToken_kw_OneMinusDstColor, kToken_kw_OneMinusDstAlpha));

                pass.destinationBlendFactor = tokenToBlendFactor(token.getKind());

                advance();

                if (token.is(kToken_comma)) {
                    advance();
                    CHECK(expectOneOf(kToken_kw_Off, kToken_kw_One, kToken_kw_Zero, kToken_kw_SrcColor,
                                      kToken_kw_SrcAlpha, kToken_kw_SrcAlphaSaturate, kToken_kw_DstColor,
                                      kToken_kw_DstAlpha, kToken_kw_OneMinusSrcColor, kToken_kw_OneMinusSrcAlpha,
                                      kToken_kw_OneMinusDstColor, kToken_kw_OneMinusDstAlpha));

                    pass.sourceAlphaFactor = tokenToBlendFactor(token.getKind());

                    advance();

                    CHECK(expectOneOf(kToken_kw_Off, kToken_kw_One, kToken_kw_Zero, kToken_kw_SrcColor,
                                      kToken_kw_SrcAlpha, kToken_kw_SrcAlphaSaturate, kToken_kw_DstColor,
                                      kToken_kw_DstAlpha, kToken_kw_OneMinusSrcColor, kToken_kw_OneMinusSrcAlpha,
                                      kToken_kw_OneMinusDstColor, kToken_kw_OneMinusDstAlpha));

                    pass.destinationAlphaFactor = tokenToBlendFactor(token.getKind());

                    advance();

                } else {
                    pass.sourceAlphaFactor = kBlendFactorOne;
                    pass.destinationAlphaFactor = kBlendFactorZero;
                }

                break;

            case kToken_kw_BlendOp:
                advance();
                CHECK(expectOneOf(kToken_kw_Add, kToken_kw_Sub, kToken_kw_RevSub, kToken_kw_Min, kToken_kw_Max));
                pass.blendOperation = tokenToBlendOperation(token.getKind());
                advance();
                break;

            case kToken_kw_ColorMask:
                advance();

                CHECK(expectOneOf(kToken_integer_literal, kToken_identifier));
                if (token.is(kToken_integer_literal)) {
                    if (token.getIntegerLiteral() != 0) {
                        errorWithMsg(1, std::format("invalid ColorMask value {}", token.getIntegerLiteral()).c_str());
                        break;
                    }
                    /// 0 means no channels, which disable color write
                    pass.colorWriteMask = kColorWriteMaskNone;

                } else {
                    /// identifier like RGBA, RGB or RB
                    pass.colorWriteMask = 0;
                    std::string_view mask = token.getIdentifier();
                    for (char ch : mask) {
                        switch (ch) {
                            case 'R':
                                pass.colorWriteMask |= kColorWriteMaskRed;
                                break;
                            case 'G':
                                pass.colorWriteMask |= kColorWriteMaskGreen;
                                break;
                            case 'B':
                                pass.colorWriteMask |= kColorWriteMaskBlue;
                                break;
                            case 'A':
                                pass.colorWriteMask |= kColorWriteMaskAlpha;
                                break;
                            default:
                                errorWithMsg(1, std::format("invalid ColorMask value {}", mask).c_str());
                                break;
                        }
                    }
                }
                advance();
                break;

            case kToken_kw_ZClip:
                advance();
                CHECK(expectOneOf(kToken_kw_True, kToken_kw_False, kToken_kw_On, kToken_kw_Off));
                pass.ZClip = token.isOneOf(kToken_kw_True, kToken_kw_On);
                advance();
                break;

            case kToken_kw_ZTest:
                advance();
                CHECK(expectOneOf(kToken_kw_Less, kToken_kw_LEqual, kToken_kw_Equal,
                                  kToken_kw_GEqual, kToken_kw_Greater, kToken_kw_NotEqual, kToken_kw_Always));

                pass.ZTestOperation = tokenToCompareFunction(token.getKind());
                advance();
                break;

            case kToken_kw_ZWrite:
                advance();
                CHECK(expectOneOf(kToken_kw_True, kToken_kw_False, kToken_kw_On, kToken_kw_Off));
                pass.ZWrite = token.isOneOf(kToken_kw_True, kToken_kw_On);;
                advance();
                break;

            default:
                goto __done;
        }
    }

__done:
    return true;

__error:
    return false;
}

bool Parser::parseHLSLInclude(std::string &outInclude) {
    CHECK(consume(kToken_kw_HLSLINCLUDE));
    CHECK(expect(kToken_hlsl_source));

    outInclude = token.getRawData();
    advance();

    CHECK(consume(kToken_kw_ENDHLSL));
    return true;
__error:
    return false;
}

bool Parser::parseHLSLSource(std::string &outSource) {
    CHECK(consume(kToken_kw_HLSLPROGRAM));
    CHECK(expect(kToken_hlsl_source));

    outSource = token.getRawData();
    advance();

    CHECK(consume(kToken_kw_ENDHLSL));
    return true;
__error:
    return false;
}

ShaderInfo Parser::parse() {
    ShaderInfo shaderInfo{};
    parseShader(shaderInfo);
    return shaderInfo;
}



}