//
// Created by Aleudillonam on 7/31/2023.
//

#include "Render/Light.hpp"
#include "Components/Transform.hpp"
#include "Render/Material.hpp"

#ifdef OJOIE_WITH_EDITOR
#include "IMGUI/IMGUI.hpp"
#endif

namespace AN {

IMPLEMENT_AN_CLASS(Light)
LOAD_AN_CLASS(Light)

Light::Light(ObjectCreationMode mode) : Super(mode), m_ListNode(this), m_Color(1.f, 1.f, 1.f, 1.f), bAddToManager(), m_DepthBias(), m_NormalBias() {}

bool Light::init() {
    if (!Super::init()) return false;

    m_Type = kLightDirectional;

    GetLightManager().addLight(m_ListNode);
    bAddToManager = true;
    return true;
}

Light::~Light() {}

void Light::dealloc() {
    if (bAddToManager) {
        GetLightManager().removeLight(m_ListNode);
    }
    Super::dealloc();
}

void Light::onInspectorGUI() {
#ifdef OJOIE_WITH_EDITOR
    ItemLabel("Light Color", kItemLabelLeft);
    ImGui::ColorEdit4("##Light Color ", (float *)&m_Color);
    ItemLabel("Depth Bias", kItemLabelLeft);
    ImGui::DragFloat("##Depth Bias ", &m_DepthBias, 0.001f);
    ItemLabel("Normal Bias", kItemLabelLeft);
    ImGui::DragFloat("##Normal Bias ", &m_NormalBias, 0.001f);
#endif
}

LightManager::LightManager() {
    /// set default material value
    Material::SetVectorGlobal("_GlossyEnvironmentColor", { 0.34f, 0.33f, 0.33f, 1.f });
    Material::SetVectorGlobal("_MainLightPosition", { 0.61639f, 0.7687f, 0.1703f, 1.f });
    Material::SetVectorGlobal("_MainLightColor", { 1.f, 1.f, 1.f, 1.f });
    Material::SetIntGlobal("_MainLightLayerMask", 0x1);
    Material::SetVectorGlobal("an_LightData", { 1.f, 1.f, 1.f, 1.f });
}

void LightManager::addLight(LightListNode &node) {
    m_List.push_back(node);
}

void LightManager::removeLight(LightListNode &node) {
    node.removeFromList();
}

void LightManager::update() {
    /// get first directional light as main light
    Light *mainLight = nullptr;
    for (LightListNode &node : m_List) {
        Light &light = *node;
        if (light.getType() == kLightDirectional) {
            mainLight = &light;
            break;
        }
    }

    if (mainLight) {
        Material::SetVectorGlobal("_MainLightPosition", Vector4f{ Math::normalize(mainLight->getTransform()->getPosition()), 1.f });
        Material::SetVectorGlobal("_MainLightColor", mainLight->getColor());
    }
}

Light *LightManager::getMainLight() {
    Light *mainLight = nullptr;
    for (LightListNode &node : m_List) {
        Light &light = *node;
        if (light.getType() == kLightDirectional) {
            mainLight = &light;
            break;
        }
    }

    return mainLight;
}

LightManager &GetLightManager() {
    static LightManager lightManager;
    return lightManager;
}

}

