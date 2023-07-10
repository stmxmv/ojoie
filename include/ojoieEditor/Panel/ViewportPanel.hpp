//
// Created by aojoie on 10/6/2022.
//

#ifndef OJOIE_VIEWPORTPANEL_HPP
#define OJOIE_VIEWPORTPANEL_HPP

#include "EditorPanel.hpp"
#include "ojoie/UI/Imgui.hpp"
#include <ojoie/Render/RenderQueue.hpp>

namespace AN {

extern RC::Texture &__getViewportTexture();

}

namespace AN::Editor {

class ViewportPanel : public Panel {
    typedef ViewportPanel Self;
    typedef Panel Super;
    bool show;
    ImVec2 size;
    bool r_isFocus;

    /// game data

    enum class InputState {
        None = 0,
        CharacterMove,
        CenterRotate
    } _inputState;

    bool _isFocus;

    bool action;

    AN::InputBinding viewportInputBinding;
    AN::Math::vec3 moveMentInput;

    std::shared_ptr<AN::CameraNode> getCurrentCamera_recur(std::shared_ptr<Node> node) {
        for (auto && childNode : node->children()) {
            std::shared_ptr<AN::CameraNode> camera = std::dynamic_pointer_cast<AN::CameraNode>(childNode);
            if (camera) {
                return camera;
            }

            if (auto camera1 = getCurrentCamera_recur(childNode); camera1) {
                return camera1;
            }
        }
        return {};
    }

public:

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual bool init() override {
        if (!Super::init()) {
            return false;
        }
        action = false;
        show = true;
        viewportInputBinding.bindAxis("__ViewportCameraPitch", this, &Self::pitchCamera);
        viewportInputBinding.bindAxis("__ViewportCameraYaw", this, &Self::yawCamera);

        viewportInputBinding.bindAxis("__ViewportMoveForward", this, &Self::moveForward);
        viewportInputBinding.bindAxis("__ViewportMoveRight", this, &Self::moveRight);
        viewportInputBinding.bindAction("__ViewportCharacterMoveEnable", InputEvent::Pressed, this, &Self::characterMoveEnable);
        viewportInputBinding.bindAction("__ViewportCharacterMoveEnable", InputEvent::Released, this, &Self::characterMoveDisable);

        viewportInputBinding.bindAction("__ViewportCenterRotate", InputEvent::Pressed, this, &Self::centerRotateEnable);
        viewportInputBinding.bindAction("__ViewportCenterRotate", InputEvent::Released, this, &Self::centerRotateDisable);

        viewportInputBinding.bindAction("__ViewportAction", InputEvent::Pressed, this, &Self::viewportAction);
        viewportInputBinding.bindAction("__ViewportAction", InputEvent::Released, this, &Self::viewportNoAction);
        return true;
    }

    std::shared_ptr<AN::CameraNode> getCurrentCamera() {
        return getCurrentCamera_recur(GetGame().entryNode);
    }

    virtual void update(float deltaTime) override {
        Super::update(deltaTime);

        moveMentInput = {};
        GetInputManager().processInput(viewportInputBinding);

        if (_inputState == InputState::CharacterMove) {
            auto camera = getCurrentCamera();
            if (camera) {
                AN::Math::normalize(moveMentInput);
                auto position = camera->getPosition();
                auto rotation = camera->getRotation();

                auto rotationMatrix = AN::Math::eulerAngleYXZ(
                        AN::Math::radians(rotation.yaw),
                        AN::Math::radians(rotation.pitch),
                        0.f);
                AN::Math::vec3 forwardVector = rotationMatrix * AN::Math::vec4(AN::CameraNode::GetDefaultForwardVector(), 1.f);
                forwardVector = AN::Math::normalize(forwardVector);
                AN::Math::vec3 rightVector = rotationMatrix * AN::Math::vec4(AN::CameraNode::GetDefaultRightVector(), 1.f);
                rightVector = AN::Math::normalize(rightVector);
                position += forwardVector * moveMentInput.x * 100.f * deltaTime;
                position += rightVector * moveMentInput.y * 100.f * deltaTime;

                camera->setPosition(position);
            }
        }
    }

    void draw(const AN::RenderContext &context) {
        Super::draw(context);
        if (show) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{});
            ImGui::Begin("viewport", &show);

            auto viewportSize = ImGui::GetContentRegionAvail();
            if (size.x != viewportSize.x || size.y != viewportSize.y) {
                size = viewportSize;
                Dispatch::async(Dispatch::Game, [viewportSize, camera = getCurrentCamera()]() {
                    if (camera) {
                        camera->setViewportSize(viewportSize.x, viewportSize.y);
                    }
                });
            }

            ImGui::Image(&AN::__getViewportTexture(), { size.x, size.y });

            bool focus = ImGui::IsItemHovered();

            if (r_isFocus != focus) {
                r_isFocus = focus;
                Dispatch::async(Dispatch::Game, [focus, _self = shared_from_this()] {
                    Self *self = (Self *)_self.get();
                    self->_isFocus = focus;
                });
            }

            ImGui::End();
            ImGui::PopStyleVar();
        }
    }

    const ImVec2 &getViewportSize() const {
        return size;
    }

    void pitchCamera(float axisValue) {
        auto camera = getCurrentCamera();
        if (camera) {
            if (_inputState == InputState::CharacterMove) {
                auto rotation = camera->getRotation();
                rotation.pitch = std::clamp(rotation.pitch - axisValue, -80.0f, 80.0f);
                camera->setRotation(rotation);
            }  else if (_inputState == InputState::CenterRotate && action) {
                /// TODO position may be an object
                auto position = camera->getPosition();

                Math::vec3 normal{ 1.f, 0.f, 0.f };
                normal = Math::rotateY(normal, Math::radians(camera->getRotation().yaw));

                position = AN::Math::rotate(position, Math::radians(-axisValue), normal);
                camera->setPosition(position);

                auto rotation = camera->getRotation();
                rotation.pitch -= axisValue;
                camera->setRotation(rotation);
            }
        }
    }

    void yawCamera(float axisValue) {
        auto camera = getCurrentCamera();
        if (camera) {
            if (_inputState == InputState::CharacterMove) {
                auto rotation = camera->getRotation();
                rotation.yaw -= axisValue;
                camera->setRotation(rotation);
            } else if (_inputState == InputState::CenterRotate && action) {
                auto position = camera->getPosition();
                position = AN::Math::rotateY(position, Math::radians(-axisValue));
                camera->setPosition(position);
                auto rotation = camera->getRotation();
                rotation.yaw -= axisValue;
                camera->setRotation(rotation);
            }
        }
    }

    void moveForward(float AxisValue) {
        if (_inputState == InputState::CharacterMove) {
            moveMentInput.x = std::clamp(AxisValue, -1.0f, 1.0f);
        }
    }

    void moveRight(float AxisValue) {
        if (_inputState == InputState::CharacterMove) {
            moveMentInput.y = std::clamp(AxisValue, -1.0f, 1.0f);
        }
    }

    void characterMoveEnable() {
        if (_isFocus && _inputState != InputState::CharacterMove) {
            _inputState = InputState::CharacterMove;
            Dispatch::async(Dispatch::Main, [] {
                AN::Cursor::setState(AN::CursorState::Disabled);
            });
        }
    }

    void characterMoveDisable() {
        if (_inputState == InputState::CharacterMove) {
            _inputState = InputState::None;
            Dispatch::async(Dispatch::Main, [] {
                AN::Cursor::setState(AN::CursorState::Normal);
            });
        }
    }

    void centerRotateEnable() {
        if (_isFocus && _inputState != InputState::CenterRotate) {
            _inputState = InputState::CenterRotate;
            Dispatch::async(Dispatch::Render, [] {
                Dispatch::async(Dispatch::Main, [] {
                    AN::Cursor::setShape(AN::CursorShape::CrossHair);
                });
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            });

        }
    }

    void centerRotateDisable() {
        if (_inputState == InputState::CenterRotate) {
            _inputState = InputState::None;
            Dispatch::async(Dispatch::Render, [] {
                Dispatch::async(Dispatch::Main, [] {
                    AN::Cursor::setShape(AN::CursorShape::Arrow);
                });
                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
            });
        }
    }

    void viewportAction() {
        action = true;
    }

    void viewportNoAction() {
        action = false;
    }
};

}

#endif//OJOIE_VIEWPORTPANEL_HPP
