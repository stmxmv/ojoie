//
// Created by aojoie on 4/22/2023.
//

#ifndef OJOIE_MESHRENDERER_HPP
#define OJOIE_MESHRENDERER_HPP

#include <ojoie/Components/Transform.hpp>
#include <ojoie/Core/Component.hpp>
#include <ojoie/Render/Material.hpp>
#include <ojoie/Render/Mesh/Mesh.hpp>
#include <ojoie/Render/Renderer.hpp>

namespace AN {

class AN_API MeshRenderer : public Renderer {

    Mesh                   *_mesh;
    Transform              *transform;

    struct TransformData {
        Matrix4x4f objectToWorld;
        Matrix4x4f worldToObject;
    };

    TransformData transformData[kMaxFrameInFlight];

    DECLARE_DERIVED_AN_CLASS(MeshRenderer, Renderer);

    static void InitializeClass();

    void onAddMeshRenderer();

public:
    explicit MeshRenderer(ObjectCreationMode mode);

    /// messages

    Mesh *getMesh() const { return _mesh; }
    void setMesh(Mesh *mesh);

    /// update should called after all material prepared the pass data
    virtual void update(UInt32 frameIndex) override;

    // render is called during render pass context
    virtual void render(RenderContext &renderContext, const char *pass) override;


    virtual void onInspectorGUI() override;
};

}// namespace AN

#endif//OJOIE_MESHRENDERER_HPP
