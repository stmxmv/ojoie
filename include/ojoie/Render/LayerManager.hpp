//
// Created by aojoie on 6/23/2023.
//

#pragma once

#include <ojoie/Render/Layer.hpp>
#include <ojoie/Allocator/STLAllocator.hpp>

namespace AN {

class AN_API LayerManager {

    typedef std::vector<Layer *, STLAllocator<Layer *, alignof(void*)>> LayerContainer;
    LayerContainer _layers;

public:

    void addLayerInternal(Layer *layer);
    void removeLayerInternal(Layer *layer);

    bool hasLayer(int index) { return index + 1 <= _layers.size(); }
    Layer *getLayerAt(int index) { return _layers[index]; }
};


AN_API LayerManager &GetLayerManager();

}
