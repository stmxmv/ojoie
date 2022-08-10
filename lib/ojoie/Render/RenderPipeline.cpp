//
// Created by Aleudillonam on 8/3/2022.
//

#include "Core/Log.h"
#include "Render/RenderPipeline.hpp"
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <sstream>
#include <string>

namespace AN {

static bool checkCompileErrors(unsigned int shader, const char *type) {
    int success;
    char infoLog[1024];
    if (strcmp(type, "PROGRAM") != 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, sizeof infoLog, nullptr, infoLog);
            ANLog("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n%s", type, infoLog, " -- --------------------------------------------------- -- ");
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, sizeof infoLog, nullptr, infoLog);
            ANLog("ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s\n%s", type, infoLog, " -- --------------------------------------------------- -- ");
            return false;
        }
    }
    return true;
}


struct ShaderLibrary::Impl {
    unsigned int id;
};

ShaderLibrary::ShaderLibrary() : impl(new Impl{}) {}

ShaderLibrary::~ShaderLibrary() {
    delete impl;
}

void ShaderLibrary::deinit() {
    glDeleteShader(impl->id);
}

bool ShaderLibrary::init(ShaderLibraryType type, const char *source) {
    const char *typeString;
    switch (type) {
        case ShaderLibraryType::Vertex:
            impl->id = glCreateShader(GL_VERTEX_SHADER);
            typeString = "VERTEX";
            break;
        case ShaderLibraryType::Fragment:
            impl->id = glCreateShader(GL_FRAGMENT_SHADER);
            typeString = "FRAGMENT";
            break;
        case ShaderLibraryType::Geometry:
            impl->id = glCreateShader(GL_GEOMETRY_SHADER);
            typeString = "GEOMETRY";
            break;
    }

    glShaderSource(impl->id, 1, &source, nullptr);
    glCompileShader(impl->id);
    if (!checkCompileErrors(impl->id, typeString)) {

        return false;
    }

    return true;
}


struct RenderPipeline::Impl {
    unsigned int ID;
};

RenderPipeline::RenderPipeline() : impl(new Impl{}) {}

RenderPipeline::~RenderPipeline() {
    delete impl;
}

bool RenderPipeline::initWithPath(const char *vertexPath, const char *fragmentPath) {
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode   = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure &e) {
        ANLog("ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: %s", e.what());
        return false;
    }

    return initWithSource(vertexCode.c_str(), fragmentCode.c_str());
}

bool RenderPipeline::initWithSource(const char *vertexSource, const char *fragmentSource) {
    // 2. compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexSource, nullptr);
    glCompileShader(vertex);
    if (!checkCompileErrors(vertex, "VERTEX")) {
        return false;
    }
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentSource, nullptr);
    glCompileShader(fragment);
    if (!checkCompileErrors(fragment, "FRAGMENT")) {
        return false;
    }
    // shader Program
    impl->ID = glCreateProgram();
    glAttachShader(impl->ID, vertex);
    glAttachShader(impl->ID, fragment);
    glLinkProgram(impl->ID);
    if (!checkCompileErrors(impl->ID, "PROGRAM")) {
        return false;
    }
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return true;
}

bool RenderPipeline::init(const ShaderLibrary &vertex, const ShaderLibrary &fragment) {
    // shader Program
    impl->ID = glCreateProgram();
    glAttachShader(impl->ID, vertex.impl->id);
    glAttachShader(impl->ID, fragment.impl->id);
    glLinkProgram(impl->ID);
    if (!checkCompileErrors(impl->ID, "PROGRAM")) {
        return false;
    }

    return true;
}

void RenderPipeline::deinit() {
    glDeleteProgram(impl->ID);
}

static RenderPipeline *current_active_shader = nullptr;

RenderPipeline *RenderPipeline::Current() {
    return current_active_shader;
}

void RenderPipeline::activate() {
    glUseProgram(impl->ID);
    current_active_shader = this;
}

void RenderPipeline::setBool(const char *name, bool value) const {
    glUniform1i(glGetUniformLocation(impl->ID, name), (int)value);
}

void RenderPipeline::setInt(const char *name, int value) const {
    glUniform1i(glGetUniformLocation(impl->ID, name), value);
}

void RenderPipeline::setFloat(const char *name, float value) const {
    glUniform1f(glGetUniformLocation(impl->ID, name), value);
}
void RenderPipeline::setVec2(const char *name, const glm::vec2 &value) const {
    glUniform2fv(glGetUniformLocation(impl->ID, name), 1, &value[0]);
}
void RenderPipeline::setVec2(const char *name, float x, float y) const {
    glUniform2f(glGetUniformLocation(impl->ID, name), x, y);
}
void RenderPipeline::setVec3(const char *name, const glm::vec3 &value) const {
    glUniform3fv(glGetUniformLocation(impl->ID, name), 1, &value[0]);
}
void RenderPipeline::setVec3(const char *name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(impl->ID, name), x, y, z);
}
void RenderPipeline::setVec4(const char *name, const glm::vec4 &value) const {
    glUniform4fv(glGetUniformLocation(impl->ID, name), 1, &value[0]);
}
void RenderPipeline::setVec4(const char *name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(impl->ID, name), x, y, z, w);
}
void RenderPipeline::setMat2(const char *name, const glm::mat2 &mat) const {
    glUniformMatrix2fv(glGetUniformLocation(impl->ID, name), 1, GL_FALSE, &mat[0][0]);
}
void RenderPipeline::setMat3(const char *name, const glm::mat3 &mat) const {
    glUniformMatrix3fv(glGetUniformLocation(impl->ID, name), 1, GL_FALSE, &mat[0][0]);
}
void RenderPipeline::setMat4(const char *name, const glm::mat4 &mat) const {
    glUniformMatrix4fv(glGetUniformLocation(impl->ID, name), 1, GL_FALSE, &mat[0][0]);
}


}// namespace AN