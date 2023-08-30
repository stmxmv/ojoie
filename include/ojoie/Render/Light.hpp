//
// Created by Aleudillonam on 7/30/2023.
//

#pragma once

#include <ojoie/Core/Component.hpp>
#include <ojoie/Template/LinkedList.hpp>
#include <ojoie/Math/Math.hpp>

namespace AN {


enum LightType {
    kLightDirectional,
    kLightPoint
};

class Light;
typedef ListNode<Light> LightListNode;
typedef List<LightListNode> LightList;

class AN_API Light : public Component {

    LightListNode m_ListNode;
    LightType m_Type;
    Vector4f m_Color;
    float m_DepthBias;
    float m_NormalBias;

    bool bAddToManager;

    DECLARE_DERIVED_AN_CLASS(Light, Component)

public:

    explicit Light(ObjectCreationMode mode);
    
    virtual bool init() override;

    virtual void dealloc() override;

    LightType getType() const { return m_Type; }
    void setType(LightType type) { m_Type = type; }

    Vector4f getColor() const { return m_Color; }
    void setColor(const Vector4f &color) { m_Color = color; }

    float getDepthBias() const { return m_DepthBias; }
    void setDepthBias(float mDepthBias) { m_DepthBias = mDepthBias; }

    float getNormalBias() const { return m_NormalBias; }
    void setNormalBias(float mNormalBias) { m_NormalBias = mNormalBias; }

    virtual void onInspectorGUI() override;
};

class AN_API LightManager {
    LightList m_List;
public:

    LightManager();

    void addLight(LightListNode &node);
    void removeLight(LightListNode &node);

    void update();

    Light *getMainLight();

};

AN_API LightManager &GetLightManager();


}
