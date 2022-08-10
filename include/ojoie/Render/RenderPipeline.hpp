//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_RENDERPIPELINE_HPP
#define OJOIE_RENDERPIPELINE_HPP

#include <ojoie/Math/Math.hpp>
#include <concepts>

namespace AN {

enum class ShaderLibraryType {
    Vertex,
    Fragment,
    Geometry /// currently is not supported
};

class ShaderLibrary : private NonCopyable {
    struct Impl;
    Impl *impl;
    friend class RenderPipeline;
public:

    ShaderLibrary();

    ~ShaderLibrary();

    bool init(ShaderLibraryType type, const char *source);

    void deinit();

};

/// \brief this class can only be use on render thread
class RenderPipeline : private NonCopyable {
    struct Impl;
    Impl *impl;
public:
    RenderPipeline();

    ~RenderPipeline();

    /// \result current active shader
    static RenderPipeline *Current();

    bool initWithPath(const char* vertexPath, const char* fragmentPath);

    bool initWithSource(const char *vertexSource, const char *fragmentSource);

    bool init(const ShaderLibrary &vertex, const ShaderLibrary &fragment);

    void deinit();

    void activate();

    // utility uniform functions
    void setBool(const char *name, bool value) const;
    void setInt(const char *name, int value) const;
    void setFloat(const char *name, float value) const;
    void setVec2(const char *name, const Math::vec2 &value) const;
    void setVec2(const char *name, float x, float y) const;
    // ------------------------------------------------------------------------
    void setVec3(const char *name, const Math::vec3 &value) const;
    void setVec3(const char *name, float x, float y, float z) const;
    // ------------------------------------------------------------------------
    void setVec4(const char *name, const Math::vec4 &value) const;
    void setVec4(const char *name, float x, float y, float z, float w) const;
    // ------------------------------------------------------------------------
    void setMat2(const char *name, const Math::mat2 &mat) const;
    // ------------------------------------------------------------------------
    void setMat3(const char *name, const Math::mat3 &mat) const;
    // ------------------------------------------------------------------------
    void setMat4(const char *name, const Math::mat4 &mat) const;
    
};



}

#endif//OJOIE_RENDERPIPELINE_HPP
