//
// Created by aojoie on 4/21/2023.
//

#include "Render/Material.hpp"
#include "Render/RenderContext.hpp"
#include "Render/Texture.hpp"
#include "Threads/Dispatch.hpp"

#ifdef OJOIE_USE_VULKAN
#include "Render/private/vulkan/UniformBuffers.hpp"
#include "Render/private/vulkan/CommandBuffer.hpp"
#include "Render/private/vulkan/TextureManager.hpp"
#include "Render/private/vulkan/RenderPipelineState.hpp"
#endif//OJOIE_USE_VULKAN

#include "Render/private/D3D11/TextureManager.hpp"
#include "Render/private/D3D11/UniformBuffers.hpp"

#ifdef PropertySheet
#undef PropertySheet  /// WIN32 hack
#endif

namespace AN {

float    PropertySheet::defaultFloat = 0.0f;
Vector4f PropertySheet::defaultColor(0, 0, 0, 0);

void PropertySheet::setFloat(const FastPropertyName &name, float val) {
    // add_or_update for floats is actually slower (too small type to be efficient)
    m_Floats[name] = val;
}

void PropertySheet::setInt(const PropertySheet::FastPropertyName &name, UInt32 val) {
    m_Ints[name] = val;
}

void PropertySheet::setVectorIndexed(const FastPropertyName &name, int index, float value) {
    Vectors::iterator lb = m_Vectors.find(name);
    if (lb != m_Vectors.end()) {
        // lb points to a pair with the given key, update pair's value
        lb->second[index] = value;
    } else {
        Vector4f defaultVector = Vector4f(0.0F, 0.0F, 0.0F, 0.0F);
        defaultVector[index]   = value;
        // no key exists, insert new pair
        m_Vectors.insert(lb, std::make_pair(name, defaultVector));
    }
}

void PropertySheet::setVector(FastPropertyName name, const float r[4]) {
    m_Vectors[name] = Vector4f(r[0], r[1], r[2], r[3]);
}

void PropertySheet::setVector(FastPropertyName name, float x, float y, float z, float w) {
    m_Vectors[name] = Vector4f(x, y, z, w);
}

void PropertySheet::setValueProp(const FastPropertyName &name, int count, const float *val) {
    ANAssert(count >= 1);

    ValueProps::iterator lb = m_ValueProps.find(name);
    if (lb != m_ValueProps.end()) {
        ValueProperty &prop = lb->second;
        if (prop.count >= count) {
            // old had more space, copy new value into old place
            memcpy(m_ValueBuffer.data() + prop.offset, val, count * sizeof(float));
        } else {
            // old has less space, so allocate space at the end
            prop.offset = m_ValueBuffer.size();
            m_ValueBuffer.resize(m_ValueBuffer.size() + count);
            memcpy(m_ValueBuffer.data() + prop.offset, val, count * sizeof(float));
        }
        prop.count = count;
    } else {
        // no key exists, insert new
        ValueProperty prop;
        prop.offset = m_ValueBuffer.size();
        prop.count  = count;
        m_ValueProps.insert(lb, std::make_pair(name, prop));

        // copy value into buffer
        m_ValueBuffer.resize(m_ValueBuffer.size() + count);
        memcpy(m_ValueBuffer.data() + prop.offset, val, count * sizeof(float));
    }
}

void PropertySheet::setTexture(const FastPropertyName &name, Texture *tex) {
    TextureProperty &te = m_Textures[name];
    //    setupTextureProperties (name, te);
    te.texID = tex->getTextureID();
}

const float &PropertySheet::getFloat(const FastPropertyName &name) const {
    Floats::const_iterator i = m_Floats.find(name);
    if (i == m_Floats.end()) {
        return defaultFloat;
    }
    return i->second;
}

void PropertySheet::setColorTag(const FastPropertyName &name) {
    m_IsColorTag.insert(name);
}

bool PropertySheet::getColorTag(const FastPropertyName &name) const {
    return m_IsColorTag.count(name) == 1;
}

bool PropertySheet::hasProperty(const FastPropertyName &name) const {
    if (m_Floats.contains(name))
        return true;
    if (m_Vectors.contains(name))
        return true;
    if (m_ValueProps.contains(name))
        return true;
    if (m_Textures.contains(name))
        return true;
    return false;
}

const Vector4f &PropertySheet::getVector(const FastPropertyName &name) const {
    Vectors::const_iterator i = m_Vectors.find(name);
    if (i == m_Vectors.end()) {
        return defaultColor;
    }
    return i->second;
}

const float *PropertySheet::getValueProp(const FastPropertyName &name, int *outCount) const {
    ValueProps::const_iterator i = m_ValueProps.find(name);
    if (i == m_ValueProps.end())
        return nullptr;
    const ValueProperty &prop = i->second;
    *outCount                 = prop.count;
    ANAssert(prop.offset + prop.count <= m_ValueBuffer.size());
    return m_ValueBuffer.data() + prop.offset;
}

PropertySheet::TextureProperty *PropertySheet::getTextureProperty(const FastPropertyName &name) {
    PropertySheet::TexEnvs::iterator i = m_Textures.find(name);
    if (i == m_Textures.end())
        return nullptr;
    return &i->second;
}

const UInt32 *PropertySheet::findInt(const PropertySheet::FastPropertyName &name) const {
    Ints::const_iterator i = m_Ints.find(name);
    if (i == m_Ints.end())
        return nullptr;
    return &i->second;
}

const float *PropertySheet::findFloat(const PropertySheet::FastPropertyName &name) const {
    Floats::const_iterator i = m_Floats.find(name);
    if (i == m_Floats.end())
        return nullptr;
    return &i->second;
}

const Vector4f *PropertySheet::findVector(const PropertySheet::FastPropertyName &name) const {
    Vectors::const_iterator i = m_Vectors.find(name);
    if (i == m_Vectors.end())
        return nullptr;
    return &i->second;
}

int PropertySheet::getMemoryUsage() const {
    return sizeof(PropertySheet) +
           m_Floats.size() * (sizeof(float) + sizeof(FastPropertyName)) +
           m_IsColorTag.size() * (sizeof(int) + sizeof(FastPropertyName)) +
           m_Vectors.size() * (sizeof(Vector4f) + sizeof(FastPropertyName)) +
           m_ValueProps.size() * (sizeof(ValueProperty) + sizeof(FastPropertyName)) +
           m_ValueBuffer.size() +
           m_Textures.size() * (sizeof(TextureProperty) + sizeof(FastPropertyName));
}


IMPLEMENT_AN_CLASS(Material);
LOAD_AN_CLASS(Material);


/// this helper function will ensure func() run in render thread
template<typename _Func>
void RunInRenderQueue(_Func &&func) {
    if (GetCurrentThreadID() != Dispatch::GetThreadID(Dispatch::Render) && Dispatch::IsRunning(Dispatch::Render)) {
        Dispatch::async(Dispatch::Render, std::forward<_Func>(func));
    } else {
        func();
    }
}

static PropertySheet gPropertySheet;

Material::~Material() {}

Material::Material(ObjectCreationMode mode) : Super(mode) {}

bool Material::init(Shader *shader, std::string_view name) {
    if (!Super::init()) return false;
    _shader = shader;
    setName(name);

    return true;
}

void Material::setMatrix(Name name, const Matrix4x4f &val) {
    _propertySheet.setValueProp(name, 16, Math::value_ptr(val));
}

void Material::setVector(Name name, const Vector4f &vector) {
    _propertySheet.setVector(name, Math::value_ptr(vector));
}

void Material::setTexture(Name name, Texture *val) {
    _propertySheet.setTexture(name, val);
}

void Material::setInt(Name name, UInt32 value) {
    _propertySheet.setInt(name, value);
}

void Material::setFloat(Name name, float value) {
    _propertySheet.setFloat(name, value);
}

void Material::SetMatrixGlobal(Name name, const Matrix4x4f &val) {
    gPropertySheet.setValueProp(name, 16, Math::value_ptr(val));
}
void Material::SetVectorGlobal(Name name, const Vector4f &vector) {
    gPropertySheet.setVector(name, Math::value_ptr(vector));
}
void Material::SetTextureGlobal(Name name, Texture *val) {
    gPropertySheet.setTexture(name, val);
}

void Material::SetIntGlobal(Name name, UInt32 value) {
    gPropertySheet.setInt(name, value);
}

void Material::SetFloatGlobal(Name name, float value) {
    gPropertySheet.setFloat(name, value);
}

void Material::applyMaterial(AN::CommandBuffer *_commandBuffer, UInt32 pass) {
    if (_shader == nullptr) return;

    AN::CommandBuffer *commandBuffer = (AN::CommandBuffer *)_commandBuffer;

    RenderPipelineState &renderPipelineState =
            _shader->getPassRenderPipelineState(pass, 0); /// TODO currently only support subPass 0

    /// set render pipeline state
    commandBuffer->setRenderPipelineState(renderPipelineState);

    /// default use subShader 0
    constexpr int subShader = 0;

    D3D11::GetUniformBuffers().resetBinds();
    for (const auto &prop : _shader->getProperties(pass, subShader)) {
        switch (prop.propertyType) {
            case kShaderPropertyFloat:
            {
                const Shader::BindingInfo *bindingInfo = _shader->getUniformBufferInfo(pass, subShader, prop.stage, prop.binding, prop.set);
                int idx = D3D11::GetUniformBuffers().findAndBind(bindingInfo->name.getIndex(), bindingInfo->stage, bindingInfo->binding, bindingInfo->size);
                if (prop.dimension == 1) {
                    const float *val = _propertySheet.findFloat(prop.name);
                    if (!val) {
                        val = gPropertySheet.findFloat(prop.name);
                    }
                    if (!val) goto __notFound;
                    D3D11::GetUniformBuffers().setConstant(idx, prop.offset, val, sizeof(*val));
                } else if (prop.dimension == 4) {

                    const Vector4f *val = _propertySheet.findVector(prop.name);
                    if (!val) {
                        val = gPropertySheet.findVector(prop.name);
                    }
                    if (!val) goto __notFound;
                    D3D11::GetUniformBuffers().setConstant(idx, prop.offset, val, sizeof(*val));

                } else {

                    int num;
                    const float *val = _propertySheet.getValueProp(prop.name, &num);
                    if (!val) {
                        val = gPropertySheet.getValueProp(prop.name, &num);
                    }
                    if (num != prop.dimension || val == nullptr) goto __notFound;
                    D3D11::GetUniformBuffers().setConstant(idx, prop.offset, val, sizeof(float) * num);
                }
            }
                break;
            case kShaderPropertyMatrix:
            {
                const Shader::BindingInfo *bindingInfo = _shader->getUniformBufferInfo(pass, subShader, prop.stage, prop.binding, prop.set);
                int idx = D3D11::GetUniformBuffers().findAndBind(bindingInfo->name.getIndex(), bindingInfo->stage, bindingInfo->binding, bindingInfo->size);

                int num;
                const float *val = _propertySheet.getValueProp(prop.name, &num);
                if (!val) {
                    val = gPropertySheet.getValueProp(prop.name, &num);
                }
                if (num != prop.dimension * prop.dimension || val == nullptr) goto __notFound;
                D3D11::GetUniformBuffers().setConstant(idx, prop.offset, val, sizeof(float) * num);
            }
            break;

            case kShaderPropertyInt:
            {
                const Shader::BindingInfo *bindingInfo = _shader->getUniformBufferInfo(pass, subShader, prop.stage, prop.binding, prop.set);
                int idx = D3D11::GetUniformBuffers().findAndBind(bindingInfo->name.getIndex(), bindingInfo->stage, bindingInfo->binding, bindingInfo->size);
                const UInt32 *val = _propertySheet.findInt(prop.name);
                if (!val) {
                    val = gPropertySheet.findInt(prop.name);
                }
                if (!val) goto __notFound;
                D3D11::GetUniformBuffers().setConstant(idx, prop.offset, val, sizeof(*val));
            }
                break;
            case kShaderPropertyBool:
            case kShaderPropertyStruct:
                /// we are ignoring struct
                continue;
            case kShaderPropertyTexture:
            {
                PropertySheet::TextureProperty *texProp = _propertySheet.getTextureProperty(prop.name);
                if (!texProp) {
                    texProp = gPropertySheet.getTextureProperty(prop.name);
                }
                if (!texProp) goto __notFound;
                /// TODO try to set a default texture here

                commandBuffer->bindTexture(prop.binding, texProp->texID, prop.stage);
            }
            break;

            case kShaderPropertySampler:
            {
                /// we set sampler above
                /// sampler name is sampler##tex
                std::string_view texName = prop.name.string_view();
                texName.remove_prefix(7);
                PropertySheet::TextureProperty *texProp = _propertySheet.getTextureProperty(texName);

                if (!texProp) {
                    texProp = gPropertySheet.getTextureProperty(prop.name);
                }

                if (!texProp) {
                    commandBuffer->bindSampler(prop.binding, Texture::DefaultSamplerDescriptor(), prop.stage);
                } else {
                    auto tex = D3D11::GetTextureManager().getTexture(texProp->texID);
                    commandBuffer->bindSampler(prop.binding, tex->samplerDescriptor, prop.stage);
                }

                /// TODO figure out sampler is set or not
            }
                continue;
            default:
                ANAssert(false && "invalid property type");
        }

        continue;

    __notFound:
        AN_LOG(Error, "property name %s not assign in material %s", prop.name.c_str(), getName().c_str());
    }
}

}// namespace AN