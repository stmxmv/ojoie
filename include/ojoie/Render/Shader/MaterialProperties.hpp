//
// Created by aojoie on 4/14/2023.
//

#ifndef OJOIE_MATERIALPROPERTIES_HPP
#define OJOIE_MATERIALPROPERTIES_HPP

#include <ojoie/Core/Name.hpp>
#include <ojoie/Math/Math.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <vector>

namespace AN {



class MaterialPropertyBlock {
    struct Property {
        Name name;
        UInt8	rows;
        UInt8	cols;
        UInt8	texDim; // if texDim==None, this is a value property
        // These should not be size_t, as the GfxDevice may run across processes of different
        // bitness, and the data serialized in the command buffer must match.
        UInt32	arraySize;
        UInt32	offset;
    };

    std::vector<Property> _properties;
    std::vector<float> _buffer;
public:



};


}// namespace AN

#endif//OJOIE_MATERIALPROPERTIES_HPP
