//
// Created by aojoie on 4/12/2023.
//

#include "Object/NamedObject.hpp"

namespace AN {

IMPLEMENT_AN_CLASS(NamedObject);
LOAD_AN_CLASS(NamedObject);

NamedObject::~NamedObject() = default;

template<typename _Coder>
void NamedObject::transfer(_Coder &coder) {
    Super::transfer(coder);
    TRANSFER(_name);
}

IMPLEMENT_AN_OBJECT_SERIALIZE(NamedObject)
INSTANTIATE_TEMPLATE_TRANSFER(NamedObject)

bool NamedObject::init() {
    if (!Super::init()) return false;
    setName(getClassName());
    return true;
}

bool NamedObject::init(Name name) {
    if (!Super::init()) return false;
    setName(name);
    return true;
}

}