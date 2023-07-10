//
// Created by aojoie on 4/12/2023.
//

#ifndef OJOIE_NAMEDOBJECT_HPP
#define OJOIE_NAMEDOBJECT_HPP

#include <ojoie/Object/Object.hpp>
#include <ojoie/Core/Name.hpp>

namespace AN {

class AN_API NamedObject : public Object {
    Name _name;

    DECLARE_DERIVED_ABSTRACT_AN_CLASS(NamedObject, Object)
    DECLARE_OBJECT_SERIALIZE(NamedObject)

    explicit NamedObject(ObjectCreationMode mode) : Object(mode) {}

    /// default set the name as class name
    virtual bool init() override;

    virtual bool init(Name name);

    virtual Name getName() const { return _name; }
    virtual void setName(Name name) { _name = name; }
};


}

#endif//OJOIE_NAMEDOBJECT_HPP
