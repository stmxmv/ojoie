//
// Created by aojoie on 4/14/2023.
//

#include <gtest/gtest.h>
#include <iostream>
#include <ojoie/ShaderLab/Lexer.hpp>
#include <ojoie/Utility/SourceFile.hpp>

using namespace AN;
using namespace AN::ShaderLab;

using std::cout, std::endl;

class LexerTest : public testing::Test {

public:
    inline static SourceFile sourceFile;

    Lexer *lexer;
protected:

    static void SetUpTestSuite() {
        ASSERT_TRUE(sourceFile.init("C:\\Users\\aojoie\\CLionProjects\\ojoie\\lib\\ojoie\\Shaders\\test.shader"));
    }

    static void TearDownTestSuite() {
        sourceFile.close();
    }

    virtual void SetUp() override {
        lexer = new Lexer(sourceFile.getBuffer());
    }

    virtual void TearDown() override {
        delete lexer;
    }
};

TEST_F(LexerTest, lex) {

    Token token;
    for (;;) {

        lexer->next(token);
        if (token.is(kToken_eof)) {
            break;
        }

        cout << token.getName() << ' ' << (token.is(kToken_hlsl_source) ? "" : token.getRawData()) << endl;

    }

}
