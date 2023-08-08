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

// Properties with their current values. These are used inside the actual materials.
class AN_API PropertySheet {
    typedef Name FastPropertyName;

public:

    struct TextureProperty {
        TextureProperty() : tex(), texID(), scaleOffsetValue(NULL), texelSizeValue(NULL) {}
        Texture *tex;
        TextureID texID;

        // TODO current unused
        Vector4f *scaleOffsetValue;// name + "_ST" property
        Vector4f *texelSizeValue;  // name + "_TexelSize" property
    };

    // Other value properties, like matrices & arrays (those are not serialized on Unity side).
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
    UInt32 getInt(const FastPropertyName &name);

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

    /// property sheet don't retain Texture
    void setTexture(const FastPropertyName &name, Texture *tex);


    bool hasProperty(const FastPropertyName &name) const;

    //     Lerps color & float properties from two other sheets.
    //    void LerpProperties(const PropertySheet &src1, const PropertySheet &src2, float alpha);

    const UInt32   *findInt(const FastPropertyName &name) const;
    const float    *findFloat(const FastPropertyName &name) const;
    const Vector4f *findVector(const FastPropertyName &name) const;

    // Marks a Vector4 as a Color.
    // This is used to perform color conversion when assigning colors to the property sheet.
    void setColorTag(const FastPropertyName &name);
    bool getColorTag(const FastPropertyName &name) const;


    const Floats  &getFloatsMap() const { return m_Floats; }
    const Vectors &getVectorMap() const { return m_Vectors; }
    const TexEnvs &getTexEnvsMap() const { return m_Textures; }

    Floats  &getFloatsMap() { return m_Floats; }
    Vectors &getVectorMap() { return m_Vectors; }
    TexEnvs &getTexEnvsMap() { return m_Textures; }

    int getMemoryUsage() const;

    static float    defaultFloat;
    static Vector4f defaultColor;
};


class AN_API Material : public NamedObject {

    PropertySheet         _propertySheet; /// render thread access only
    Shader *_shader;


    DECLARE_DERIVED_AN_CLASS(Material, NamedObject);

public:

    explicit Material(ObjectCreationMode mode);

    virtual bool init(Shader *shader, std::string_view name);

    void setShader(Shader *shader);
    Shader *getShader() const { return _shader; }

    const PropertySheet &getPropertySheet() const { return _propertySheet; }

    static void SetReplacementShader(Shader *shader, const char *RenderType = nullptr);

    static void SetMatrixGlobal(Name name, const Matrix4x4f &val);
    static void SetVectorGlobal(Name name, const Vector4f &vector);
    static void SetTextureGlobal(Name name, Texture *val);
    static void SetIntGlobal(Name name, UInt32 value);
    static void SetFloatGlobal(Name name, float value);

    /// game thread setting material properties
    void setMatrix(Name name, const Matrix4x4f &val);
    void setInt(Name name, UInt32 value);
    void setFloat(Name name, float value);
    void setVector(Name name, const Vector4f &vector);
    void setTexture(Name name, Texture *val);

    bool hasPass(const char *pass);

    void applyMaterial(AN::CommandBuffer *commandBuffer, const char *pass);

    /// this method must be called during render pass
    /// apply properties, thus set uniform buffer or texture in pipeline
    void applyMaterial(AN::CommandBuffer *commandBuffer, UInt32 pass);

#ifdef OJOIE_WITH_EDITOR
    void onInspectorGUI();
#endif//OJOIE_WITH_EDITOR
};


}// namespace AN

#endif//OJOIE_MATERIAL_HPP
