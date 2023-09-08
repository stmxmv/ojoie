
#include "Render/Renderer.hpp"
#include "Core/Actor.hpp"
#include "Render/RenderManager.hpp"
#include <unordered_set>

namespace AN {


IMPLEMENT_AN_CLASS_INIT(Renderer);
LOAD_AN_CLASS(Renderer);

Renderer::Renderer(ObjectCreationMode mode) : Super(mode), bAddToManager() {}

Renderer::~Renderer() {}

void Renderer::InitializeClass() {
    GetClassStatic()->registerMessageCallback(kDidAddComponentMessage,
                                              [](void *receiver, Message &message) {
                                                  Renderer *renderer = (Renderer *) receiver;
                                                  if (message.getData<Component *>() == renderer) {
                                                      /// only do when the added component is self
                                                      renderer->onAddRenderer();
                                                  }
                                              });
}

void Renderer::onAddRenderer() {
    bAddToManager = true;
    GetRenderManager().addRenderer(_rendererListNode);
}

void Renderer::setMaterial(UInt32 index, Material *material) {
    if (_materials.size() < index + 1) {
        _materials.resize(index + 1, material);
    } else {
        _materials[index] = material;
    }
}

void Renderer::dealloc() {
    if (bAddToManager) {
        GetRenderManager().removeRenderer(_rendererListNode);
    }
    Super::dealloc();
}


}