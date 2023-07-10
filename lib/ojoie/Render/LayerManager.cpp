//
// Created by aojoie on 6/23/2023.
//

#include "Render/LayerManager.hpp"

namespace AN {

void LayerManager::addLayerInternal(Layer *layer) {
    _layers.push_back(layer);
    layer->setIndexInternal(_layers.size() - 1);
}

void LayerManager::removeLayerInternal(Layer *layer) {
    /// make index available but return null
    for (Layer * &aLayer : _layers) {
        if (aLayer == layer) {
            aLayer = nullptr;
        }
    }
}

LayerManager &GetLayerManager() {
    static LayerManager layerManager;
    return layerManager;
}

}