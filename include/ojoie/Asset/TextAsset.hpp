//
// Created by Aleudillonam on 8/2/2023.
//

#pragma once

#include <ojoie/Object/NamedObject.hpp>

namespace AN {

class AN_API TextAsset : public NamedObject {

    DECLARE_DERIVED_ABSTRACT_AN_CLASS(TextAsset, NamedObject)
public:
    explicit TextAsset(ObjectCreationMode mode) : Super(mode) {}

    virtual std::string getTextAssetPath() = 0;

    virtual void setTextAssetPath(std::string_view path) = 0;

};


}
