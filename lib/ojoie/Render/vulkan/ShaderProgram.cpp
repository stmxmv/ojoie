//
// Created by aojoie on 9/20/2022.
//

#include "Render/private/vulkan/ShaderProgram.hpp"

#include <format>

namespace AN::VK {


//bool ShaderProgram::initCommon() {
//    // Collect and combine all the shader resources from each of the shader modules
//    // Collate them all into a map that is indexed by the name of the resource
//    for (auto &function : functions) {
//        for (const auto &shader_resource : function.getShaderResources()) {
//            std::string key = std::format("{}_{}_{}", shader_resource.name, shader_resource.set, shader_resource.binding);
//
//            // Since 'Input' and 'Output' resources can have the same name, we modify the key string
//            if (shader_resource.type == ShaderResourceType::Input || shader_resource.type == ShaderResourceType::Output) {
//                key = std::to_string(shader_resource.stages) + "_" + key;
//            }
//
//            auto it = resources.find(key);
//
//            if (it != resources.end()) {
//                // Append stage flags if resource already exists
//                it->second.stages |= shader_resource.stages;
//            } else {
//                // Create a new entry in the map
//                resources.emplace(key, shader_resource);
//            }
//        }
//    }
//
//    // Sift through the map of name indexed shader resources
//    // Seperate them into their respective sets
//    for (auto &it : resources) {
//        auto &shader_resource = it.second;
//
//        // Find binding by set index in the map.
//        auto it2 = sets.find(shader_resource.set);
//
//        if (it2 != sets.end()) {
//            // Add resource to the found set index
//            it2->second.push_back(shader_resource);
//        } else {
//            // Create a new set index and with the first resource
//            sets.emplace(shader_resource.set, std::vector<ShaderResource>{shader_resource});
//        }
//    }
//
//    return true;
//}
//
//std::vector<ShaderResource> ShaderProgram::getResources(const ShaderResourceType &type, VkShaderStageFlagBits stage) const {
//    std::vector<ShaderResource> found_resources;
//
//    for (auto &it : resources) {
//        auto &shader_resource = it.second;
//
//        if (shader_resource.type == type || type == ShaderResourceType::All) {
//            if (shader_resource.stages == stage || stage == VK_SHADER_STAGE_ALL) {
//                found_resources.push_back(shader_resource);
//            }
//        }
//    }
//
//    return found_resources;
//}

}