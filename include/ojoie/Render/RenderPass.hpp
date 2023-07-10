//
// Created by aojoie on 4/21/2023.
//

#ifndef OJOIE_RENDERPASS_HPP
#define OJOIE_RENDERPASS_HPP

#include <ojoie/Render/RenderTypes.hpp>

namespace AN {

struct RenderPassImpl {
    virtual ~RenderPassImpl() = default;

    virtual bool init(const RenderPassDescriptor &renderPassDescriptor) = 0;

    virtual void deinit() = 0;
};

class AN_API RenderPass : public NonCopyable {
    RenderPassImpl *impl{};
public:

    ~RenderPass() {
        deinit();
    }

    bool init(const RenderPassDescriptor &renderPassDescriptor);

    void deinit();

    RenderPassImpl *getImpl() const { return impl; }
};


}

#endif//OJOIE_RENDERPASS_HPP
