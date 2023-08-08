//
// Created by aojoie on 4/14/2023.
//

#ifndef OJOIE_PROPERTY_HPP
#define OJOIE_PROPERTY_HPP

#include <ojoie/Core/Name.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <vector>

namespace AN::ShaderLab {


struct Property {
    Name               name;
    std::string        description;
    ShaderPropertyType type;
    int                dimension;

    bool color;

    struct {
        union {
            int   int_min;
            float float_min;
        };
        union {
            int   int_max;
            float float_max;
        };
    } range; /// if type is int or float and dimension is 1

    union {
        float       floatValue;
        int         intValue;
        Vector4f    vector4f;
    } defaultValue;

    std::string defaultStringValue; /// can be a texture name

    std::vector<Name> attributes;

    inline static const char* GetTypeString() { return "ShaderLabProperty"; }

    template<typename Coder>
    void transfer(Coder &coder) {
        TRANSFER(name);
        TRANSFER(description);
        TRANSFER(type);
        TRANSFER(dimension);
        TRANSFER(color);

        if (dimension == 1 && (type == kShaderPropertyInt || type == kShaderPropertyFloat)) {
            if (type == kShaderPropertyInt) {
                TRANSFER(range.int_min);
                TRANSFER(range.int_max);
            } else {
                /// float
                TRANSFER(range.float_min);
                TRANSFER(range.float_max);
            }
        }

        if (dimension == 1 && type == kShaderPropertyInt) {
            TRANSFER(defaultValue.intValue);
        } else if (dimension == 1 && type == kShaderPropertyFloat) {
            TRANSFER(defaultValue.floatValue);
        } else if (dimension == 4 && type == kShaderPropertyFloat) {
            TRANSFER(defaultValue.vector4f);
        }
        TRANSFER(defaultStringValue);
        TRANSFER(attributes);
    }

};


}// namespace AN::ShaderLab

#endif//OJOIE_PROPERTY_HPP
