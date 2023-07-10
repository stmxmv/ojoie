//
// Created by Aleudillonam on 8/3/2022.
//

#ifndef OJOIE_RENDERPIPELINESTATE_HPP
#define OJOIE_RENDERPIPELINESTATE_HPP


#include <ojoie/Math/Math.hpp>
#include <ojoie/Render/PipelineReflection.hpp>
#include <ojoie/Render/RenderPass.hpp>
#include <ojoie/Render/RenderTypes.hpp>

#include <map>
#include <vector>

namespace AN {

class RenderPipeline;/// this just a bridge object
struct RenderPipelineStateImpl {

    virtual ~RenderPipelineStateImpl() = default;

    virtual bool init(const RenderPipelineStateDescriptor &renderPipelineDescriptor,
                      const PipelineReflection            &reflection) = 0;

    virtual void deinit() = 0;
};

class RenderPipeline;

/// this object can be created in any thread, methods are not thread-safe
class AN_API RenderPipelineState : private NonCopyable {
    RenderPipelineStateImpl *impl{};

public:
    RenderPipelineState();

    RenderPipelineState(RenderPipelineState &&other) noexcept : impl(other.impl) {
        other.impl = nullptr;
    }

    RenderPipelineState &operator= (RenderPipelineState &&other) noexcept {
        other.swap(*this);
        return *this;
    }

    void swap(RenderPipelineState &other) {
        std::swap(impl, other.impl);
    }

    ~RenderPipelineState();

    bool init(const RenderPipelineStateDescriptor &renderPipelineDescriptor,
              const PipelineReflection            &reflection);

    void deinit();

    RenderPipelineStateImpl *getImpl() const { return impl; }
};


}// namespace AN

#endif//OJOIE_RENDERPIPELINESTATE_HPP
