//
// Created by Aleudillonam on 7/28/2022.
//

#include "Core/Node.hpp"
#include "Core/Game.hpp"
#include "Render/RenderContext.hpp"

namespace AN {



bool Node::init() {
    return true;
}


void Node::addChild(const std::shared_ptr<Node> &child) {
    _children.insert(child);
    GetGame().needsRecollectNodes = true;
}



void Node::destroy() {
    auto par = parent.lock();
    if (!par) {
        return;
    }
    par->_children.erase(shared_from_this());
    GetGame().needsRecollectNodes = true;
}



}