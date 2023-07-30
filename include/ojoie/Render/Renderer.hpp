//
// Created by Aleudillonam on 7/28/2022.
//

#ifndef OJOIE_RENDERER_HPP
#define OJOIE_RENDERER_HPP

#include <ojoie/Core/Component.hpp>
#include <ojoie/Render/Material.hpp>
#include <ojoie/Template/LinkedList.hpp>

namespace AN {

class Renderer;
typedef ListNode<Renderer> RendererListNode;
typedef List<RendererListNode> RendererList;

class AN_API Renderer : public Component {

    bool bAddToManager;
    RendererListNode _rendererListNode{ this };

protected:

    std::vector<Material *> _materials;

    virtual void onAddRenderer();

public:

    DECLARE_DERIVED_ABSTRACT_AN_CLASS(Renderer, Component);
    explicit Renderer(ObjectCreationMode mode);

    virtual void dealloc() override;

    static void InitializeClass();

    void setMaterial(UInt32 index, Material *material);

    std::span<Material *const> getMaterials() const { return _materials; }

    /// update should called after all material prepared the pass data
    virtual void update(UInt32 frameIndex) = 0;

    // render is called during render pass context
    virtual void render(RenderContext &renderContext, const char *pass) = 0;

};

}

#endif//OJOIE_RENDERER_HPP
