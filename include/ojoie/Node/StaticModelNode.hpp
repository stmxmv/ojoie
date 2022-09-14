//
// Created by Aleudillonam on 8/7/2022.
//

#ifndef OJOIE_STATICMODELNODE_HPP
#define OJOIE_STATICMODELNODE_HPP

#include <ojoie/Node/Node3D.hpp>
#include <ojoie/Node/StaticMeshNode.hpp>

namespace AN {



class StaticModelNode : public Node3D {
    typedef StaticModelNode Self;
    typedef Node3D Super;

    struct Impl;
    Impl *impl;

    virtual bool init() override { return false; }
public:

    StaticModelNode();

    ~StaticModelNode();

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual bool init(const char *modelPath);


    virtual void render(const RenderContext &context) override;
};


}


#endif//OJOIE_STATICMODELNODE_HPP
