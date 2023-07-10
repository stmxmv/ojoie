//
// Created by aojoie on 4/21/2023.
//

#ifndef OJOIE_MATERIAL_HPP
#define OJOIE_MATERIAL_HPP

#include <ojoie/Allocator/STLAllocator.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <ojoie/Render/Shader/Shader.hpp>
#include <ojoie/Render/Texture2D.hpp>
#include <ojoie/Render/UniformBuffers.hpp>

#include <unordered_map>
#include <unordered_set>

namespace AN {

class CommandBuffer;
struct RenderContext;

class AN_API PropertySheet {
    typedef Name FastPropertyName;

public:
 
    struct TextureProperty {
        TextureProperty() : texID(), scaleOffsetValue(NULL), texelSizeValue(NULL) {}
        TextureID texID;

        Vector4f *scaleOffsetValue;
        Vector4f *texelSizeValue; 
    };

    struct ValueProperty {
        UInt32 offset;
        UInt32 count;
    };

private:

    template<typename K, typename V>
    using ANMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, STLAllocator<std::pair<const K, V>, alignof(std::pair<const K, V>)>>;

    template<typename T>
    using ANSet = std::unordered_set<T, std::hash<T>, std::equal_to<T>, STLAllocator<T, alignof(T)>>;

    typedef ANMap<FastPropertyName, ValueProperty>   ValueProps;
    typedef ANMap<FastPropertyName, float>           Floats;
    typedef ANMap<FastPropertyName, UInt32>          Ints;
    typedef ANMap<FastPropertyName, Vector4f>        Vectors;
    typedef ANSet<FastPropertyName>                  IsColorTag;
    typedef ANMap<FastPropertyName, TextureProperty> TexEnvs;

    Ints               m_Ints;
    Floats             m_Floats;
    Vectors            m_Vectors;
    ValueProps         m_ValueProps;
    TexEnvs            m_Textures;
    IsColorTag         m_IsColorTag;
    std::vector<float> m_ValueBuffer;

public:

    void setInt(const FastPropertyName &name, UInt32 val);

    void         setFloat(const FastPropertyName &name, float val);
    const float &getFloat(const FastPropertyName &name) const;

    void            setVector(FastPropertyName name, const float r[4]);
    void            setVector(FastPropertyName name, float x, float y, float z, float w);
    void            setVectorIndexed(const FastPropertyName &name, int index, float value);
    const Vector4f &getVector(const FastPropertyName &name) const;

    void         setValueProp(const FastPropertyName &name, int count, const float *val);
    const float *getValueProp(const FastPropertyName &name, int *outCount) const;

    TextureProperty *getTextureProperty(const FastPropertyName &name);
    void             setTextureProperty(const FastPropertyName &name, const TextureProperty &info) { m_Textures[name] = info; }

    void setTexture(const FastPropertyName &name, Texture *tex);


    bool hasProperty(const FastPropertyName &name) const;


    const UInt32   *findInt(const FastPropertyName &name) const;
    const float    *findFloat(const FastPropertyName &name) const;
    const Vector4f *findVector(const FastPropertyName &name) const;


    const Floats  &getFloatsMap() const { return m_Floats; }
    const Vectors &getVectorMap() const { return m_Vectors; }
    const TexEnvs &getTexEnvsMap() const { return m_Textures; }

    Floats  &getFloatsMap() { return m_Floats; }
    Vectors &getVectorMap() { return m_Vectors; }
    TexEnvs &getTexEnvsMap() { return m_Textures; }
};


class AN_API Material : public NamedObject {

    PropertySheet         _propertySheet;
    Shader *_shader;


    DECLARE_DERIVED_AN_CLASS(Material, NamedObject);

public:

    explicit Material(ObjectCreationMode mode);

    virtual bool init(Shader *shader, std::string_view name);

    Shader *getShader() const { return _shader; }

    const PropertySheet &getPropertySheet() const { return _propertySheet; }

    /// game thread setting material properties
	
    static void SetMatrixGlobal(Name name, const Matrix4x4f &val);
    static void SetVectorGlobal(Name name, const Vector4f &vector);
    static void SetTextureGlobal(Name name, Texture *val);
    static void SetIntGlobal(Name name, UInt32 value);
    static void SetFloatGlobal(Name name, float value);

    void setMatrix(Name name, const Matrix4x4f &val);
    void setInt(Name name, UInt32 value);
    void setFloat(Name name, float value);
    void setVector(Name name, const Vector4f &vector);
    void setTexture(Name name, Texture *val);


    /// this method must be called during render pass
    /// apply properties, thus set uniform buffer or texture in pipeline
    void applyMaterial(AN::CommandBuffer *commandBuffer, UInt32 pass);
};


}// namespace AN

#endif//OJOIE_MATERIAL_HPP
