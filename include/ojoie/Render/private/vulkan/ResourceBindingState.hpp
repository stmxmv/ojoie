//
// Created by aojoie on 9/20/2022.
//

#ifndef OJOIE_VK_RESOURCEBINDINGSTATE_HPP
#define OJOIE_VK_RESOURCEBINDINGSTATE_HPP

namespace AN::VK {

struct ResourceInfo {
    bool dirty;
    const Buffer *buffer;
    VkDeviceSize offset;
    VkDeviceSize range;

    const ImageView *image_view;
    const RC::Sampler *sampler;
};

template<typename T>
using ResourceBindingMap = std::map<uint32_t, std::map<uint32_t, T>>;

class ResourceSet {
    bool dirty{};
    ResourceBindingMap<ResourceInfo> resource_bindings;
public:
    void reset() {
        clear_dirty();
        resource_bindings.clear();
    }

    bool is_dirty() const {
        return dirty;
    }

    void clear_dirty() {
        dirty = false;
    }

    void clear_dirty(uint32_t binding, uint32_t array_element) {
        resource_bindings[binding][array_element].dirty = false;
    }

    void bind_buffer(const Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t binding, uint32_t array_element) {
        resource_bindings[binding][array_element].dirty  = true;
        resource_bindings[binding][array_element].buffer = &buffer;
        resource_bindings[binding][array_element].offset = offset;
        resource_bindings[binding][array_element].range  = range;
        dirty = true;
    }

    void bind_image_sampler(const ImageView &image_view, const RC::Sampler &sampler, uint32_t binding, uint32_t array_element) {
        resource_bindings[binding][array_element].dirty      = true;
        resource_bindings[binding][array_element].image_view = &image_view;
        resource_bindings[binding][array_element].sampler    = &sampler;
        dirty = true;
    }

    void bind_image(const ImageView &image_view, uint32_t binding, uint32_t array_element) {
        resource_bindings[binding][array_element].dirty      = true;
        resource_bindings[binding][array_element].image_view = &image_view;
        dirty = true;
    }

    void bind_sampler(const RC::Sampler &sampler, uint32_t binding, uint32_t array_element) {
        resource_bindings[binding][array_element].dirty      = true;
        resource_bindings[binding][array_element].sampler    = &sampler;
        dirty = true;
    }

    void bind_input(const ImageView &image_view, uint32_t binding, uint32_t array_element) {
        resource_bindings[binding][array_element].dirty      = true;
        resource_bindings[binding][array_element].image_view = &image_view;
        dirty = true;
    }

    const ResourceBindingMap<ResourceInfo> &get_resource_bindings() const {
        return resource_bindings;
    }

};

class ResourceBindingState {
    bool dirty{};
    std::unordered_map<uint32_t, ResourceSet> resource_sets;
public:
    void reset() {
        clear_dirty();
        resource_sets.clear();
    }

    bool is_dirty() const {
        return dirty;
    }

    void clear_dirty() {
        dirty = false;
    }

    void clear_dirty(uint32_t set) {
        resource_sets[set].clear_dirty();
    }

    void bind_buffer(const Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element) {
        resource_sets[set].bind_buffer(buffer, offset, range, binding, array_element);
        dirty = true;
    }

    void bind_image_sampler(const ImageView &image_view, const RC::Sampler &sampler,
                    uint32_t set, uint32_t binding, uint32_t array_element) {
        resource_sets[set].bind_image_sampler(image_view, sampler, binding, array_element);
        dirty = true;
    }

    void bind_image(const ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element) {
        resource_sets[set].bind_image(image_view, binding, array_element);
        dirty = true;
    }

    void bind_sampler(const RC::Sampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element) {
        resource_sets[set].bind_sampler(sampler, binding, array_element);
        dirty = true;
    }

    void bind_input(const ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element) {
        resource_sets[set].bind_input(image_view, binding, array_element);
        dirty = true;
    }

    const std::unordered_map<uint32_t, ResourceSet> &get_resource_sets() {
        return resource_sets;
    }

};

}// namespace AN::VK

#endif//OJOIE_VK_RESOURCEBINDINGSTATE_HPP
