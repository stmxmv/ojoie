//
// Created by Aleudillonam on 7/28/2022.
//

#include "Core/Node.hpp"
#include "Core/Game.hpp"
#include "Render/RenderContext.hpp"

namespace AN {



bool Node::init() {
    sceneProxy = nullptr;
    return true;
}


void Node::addChild(const std::shared_ptr<Node> &child) {
    _children.push_back(child);
    GetGame().needsRecollectNodes = true;
}



void Node::destroy() {
    auto par = _parent.lock();
    if (!par) {
        return;
    }
    std::erase(par->_children, shared_from_this());
    GetGame().needsRecollectNodes = true;
}



}