//
// Created by aojoie on 6/24/2023.
//

#include "Panels/ViewportPanel.hpp"
#include "MainIMGUI.hpp"

#include "Picking.hpp"

#include <ojoie/Core/Behavior.hpp>
#include <ojoie/Core/Actor.hpp>
#include <ojoie/Camera/Camera.hpp>
#include <ojoie/Core/Event.hpp>
#include <ojoie/Render/TextureLoader.hpp>
#include <ojoie/Core/DragAndDrop.hpp>
#include <ojoie/Render/RenderManager.hpp>
#include <ojoie/Core/Game.hpp>
#include <ojoie/Editor/Selection.hpp>
#include <ojoie/Input/InputManager.hpp>
#include "AppDelegate.hpp"

#include <glm/gtx/matrix_decompose.hpp>

using namespace AN;
static bool DecomposeTransform(const Matrix4x4f& transform, Vector3f& translation, Vector3f& rotation, Vector3f& scale) {
    // From glm::decompose in matrix_decompose.inl

    using namespace glm;
    using T = float;

    Matrix4x4f LocalMatrix(transform);

    // Normalize the matrix.
    if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
        return false;

    // First, isolate perspective.  This is the messiest.
    if (
            epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
            epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
            epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
    {
        // Clear the perspective partition
        LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
        LocalMatrix[3][3] = static_cast<T>(1);
    }

    // Next take care of translation (easy).
    translation = vec3(LocalMatrix[3]);
    LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

    vec3 Row[3], Pdum3;

    // Now get scale and shear.
    for (length_t i = 0; i < 3; ++i)
        for (length_t j = 0; j < 3; ++j)
            Row[i][j] = LocalMatrix[i][j];

    // Compute X scale factor and normalize first row.
    scale.x = length(Row[0]);
    Row[0] = detail::scale(Row[0], static_cast<T>(1));
    scale.y = length(Row[1]);
    Row[1] = detail::scale(Row[1], static_cast<T>(1));
    scale.z = length(Row[2]);
    Row[2] = detail::scale(Row[2], static_cast<T>(1));

    // At this point, the matrix (in rows[]) is orthonormal.
    // Check for a coordinate system flip.  If the determinant
    // is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

    rotation.y = asin(-Row[0][2]);
    if (cos(rotation.y) != 0) {
        rotation.x = atan2(Row[1][2], Row[2][2]);
        rotation.z = atan2(Row[0][1], Row[0][0]);
    }
    else {
        rotation.x = atan2(-Row[2][0], Row[1][1]);
        rotation.z = 0;
    }
    return true;
}

namespace AN::Editor {


static float easeOut(float x) {
    return 1.f - (1.f - x) * (1.f - x);
}

class SceneBehavior : public Behavior {

    ViewportPanel *scenePanel;

    float _cameraYaw; // in degrees
    float _cameraPitch = 0.f;

    Vector3f pivot{};
    float _cameraDistance = 2.f;

    Vector2f _inputMove{};
    Vector2f _inputLook{};

public:
    bool disableMovement = true;
    bool action = false;

private:

    float _cameraAnimateProgressPerSec = 4.f;
    float _cameraAnimateProgress;
    Vector3f _cameraAnimateStartPosition;
    Vector3f _cameraAnimateEndPosition;
    bool _cameraAnimating = false;
    float _cameraAnimateTime;

    static float ClampAngle(float lfAngle, float lfMin, float lfMax) {
        if (lfAngle < -360.f) lfAngle += 360.f;
        if (lfAngle > 360.f) lfAngle -= 360.f;
        return std::clamp(lfAngle, lfMin, lfMax);
    }

    static void OnMoveMessage(void *receiver, Message &message) {
        SceneBehavior *self = (SceneBehavior *) receiver;
        self->onMove(*message.getData<Vector2f *>());
    }

    static void OnLookMessage(void *receiver, Message &message) {
        SceneBehavior *self = (SceneBehavior *) receiver;
        self->onLook(*message.getData<Vector2f *>());
    }

    static void OnTerminateMessage(void *receiver, Message &message) {
        App->terminate();
    }

    static void OnNavigationEnableMessage(void *receiver, Message &message) {
        SceneBehavior *self = (SceneBehavior *)receiver;
        if (!self->scenePanel->isMouseHover() || self->action) return;
        AN::Cursor::setState(AN::kCursorStateDisabled);
        self->disableMovement = false;
    }

    static void OnNavigationDisableMessage(void *receiver, Message &message) {
        SceneBehavior *self = (SceneBehavior *)receiver;
        if (!self->disableMovement) {
            AN::Cursor::setState(AN::kCursorStateNormal);
            self->disableMovement = true;
        }
    }

    static void OnSceneActionEnableMessage(void *receiver, Message &message) {
        SceneBehavior *self = (SceneBehavior *)receiver;
        if (self->scenePanel->isMouseHover()) {
            self->action = true;
        }
    }

    static void OnSceneActionDisableMessage(void *receiver, Message &message) {
        SceneBehavior *self = (SceneBehavior *)receiver;
        self->action = false;

        if (Cursor::getShape() != CursorShape::Arrow) {
            Cursor::setShape(CursorShape::Arrow);
        }

        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    }

    static void OnSceneFocusMessage(void *receiver, Message &message) {
        SceneBehavior *self = (SceneBehavior *)receiver;
        if (self->scenePanel->isMouseHover()) {
            Actor *actor = Selection::GetActiveActor();
            if (actor) {
//                Vector3f tarPos = actor->getTransform()->getPosition();
////                Vector3f dir = GetMainIMGUI().getTransform()->getRotation() * Vector4f(0.f, 0.f, 1.f, 1.f);
////                GetMainIMGUI().getTransform()->setPosition(tarPos + 3.f * dir);
//
//                Vector3f cameraDirection = self->getTransform()->getRotation() * Vector4f(0.f, 0.f, 1.f, 1.f);
//                self->pivot = tarPos;
//                self->getTransform()->setPosition(self->pivot + cameraDirection * self->_cameraDistance);

                self->animateCamera(actor->getTransform()->getPosition());
            }
        }
    }


    static void OnToggleFullScreen(void *receiver, Message &message) {
        GetMainWindow()->setFullScreen(!GetMainWindow()->isFullScreen());
    }

    static void OnFrameCaptureMessage(void *receiver, Message &message) {
        GetRenderManager().captureNextFrame();
    }

    void animateCamera(const Vector3f &tar) {
        _cameraAnimateStartPosition = getTransform()->getPosition();
        pivot = tar;
        Vector3f cameraDirection = getTransform()->getRotation() * Vector4f(0.f, 0.f, 1.f, 1.f);
        _cameraAnimateEndPosition = pivot + cameraDirection * _cameraDistance;
        _cameraAnimating = true;
        _cameraAnimateTime = 0.f;
        _cameraAnimateProgress = 0.f;
    }

    DECLARE_DERIVED_AN_CLASS(SceneBehavior, Behavior)

public:

    void setScenePanel(ViewportPanel *panel) { scenePanel = panel; }

    /// [Tooltip("How far in degrees can you move the camera up")]
    float topClamp = 90.0f;

    /// [Tooltip("How far in degrees can you move the camera down")]
    float bottomClamp = -90.0f;

    explicit SceneBehavior(ObjectCreationMode mode) : Super(mode) {}


    static void InitializeClass() {
        GetClassStatic()->registerMessageCallback("OnTerminate", OnTerminateMessage);
        GetClassStatic()->registerMessageCallback("OnMove", OnMoveMessage);
        GetClassStatic()->registerMessageCallback("OnLook", OnLookMessage);
        GetClassStatic()->registerMessageCallback("OnNavigationEnable", OnNavigationEnableMessage);
        GetClassStatic()->registerMessageCallback("OnNavigationDisable", OnNavigationDisableMessage);
        GetClassStatic()->registerMessageCallback("OnToggleFullScreen", OnToggleFullScreen);
        GetClassStatic()->registerMessageCallback("OnFrameCapture", OnFrameCaptureMessage);

        GetClassStatic()->registerMessageCallback("OnSceneActionEnable", OnSceneActionEnableMessage);
        GetClassStatic()->registerMessageCallback("OnSceneActionDisable", OnSceneActionDisableMessage);
        GetClassStatic()->registerMessageCallback("OnSceneFocus", OnSceneFocusMessage);
    }

    virtual void start() override {
        Super::start();
        _cameraYaw = getTransform()->getEulerAngles().y;

        Vector3f cameraDirection = getTransform()->getRotation() * Vector4f(0.f, 0.f, 1.f, 1.f);
        getTransform()->setPosition(pivot + cameraDirection * _cameraDistance);
    }

    virtual void update() override {
        Super::update();

        if (_cameraAnimating) {
            float deltaTime = GetGame().deltaTime;
            float progress = deltaTime * _cameraAnimateProgressPerSec;
            _cameraAnimateProgress += progress;

            float rate = easeOut(_cameraAnimateProgress);

            Vector3f position = Math::lerp(_cameraAnimateStartPosition, _cameraAnimateEndPosition, rate);

            getTransform()->setPosition(position);
            if (_cameraAnimateProgress >= 1.f) {
                _cameraAnimating = false;
            }

            return;
        }

        if (std::abs(_inputMove.x) > 0.01f || std::abs(_inputMove.y) > 0.01f) {
            Vector3f inputDirection = Vector3f(_inputMove.x, 0.0f, _inputMove.y);

            /// forward is (0, 0, -1), CCW is positive
            float rotation = -Math::atan2(inputDirection.x, inputDirection.z);

            float targetRotation = Math::degrees(rotation) + getTransform()->getEulerAngles().y;
            float targetRotationPitch = getTransform()->getEulerAngles().x;
            if (_inputMove.y < 0) {
                targetRotationPitch = -targetRotationPitch;
            }
            Vector3f targetDirection = Math::eulerAngleYXZ(AN::Math::radians(targetRotation),
                                                           AN::Math::radians(targetRotationPitch), 0.f) * Vector4f(0.f, 0.f, -1.f, 1.f);

            Vector3f position = pivot;
            position += targetDirection * GetGame().deltaTime * 10.f;

            Vector3f cameraDirection = getTransform()->getRotation() * Vector4f(0.f, 0.f, 1.f, 1.f);
            pivot = position;

            getTransform()->setPosition(pivot + cameraDirection * _cameraDistance);

            _inputMove = {};
        }


        if (std::abs(_inputLook.x) > 0.01f || std::abs(_inputLook.y) > 0.01f) {
            //Don't multiply mouse input by Time.deltaTime
            float multiplier = 1.f;
            _cameraYaw += _inputLook.x * multiplier;
            _cameraPitch += _inputLook.y * multiplier;

            _inputLook = {};

            Quaternionf quaternion = glm::eulerAngleYXZ(Math::radians(_cameraYaw), Math::radians(_cameraPitch), 0.f);
            pivot = getTransform()->getPosition() - (Vector3f)(quaternion * Vector4f(0.f, 0.f, 1.f, 1.f)) * _cameraDistance;
        }

        if (scenePanel->isFocus()) {
            Mouse &mouse = GetInputManager().getMouse();
            if (action && mouse.getMouseLeft().isPress()) {
                Cursor::setShape("IDC_ORBIT_VIEW");
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            }
        }
        if (action) {
            Mouse &mouse = GetInputManager().getMouse();
            if (mouse.getMouseLeft().isPress()) {
                Vector2Control &deltaControl = mouse.getMouseDelta();
                _cameraPitch += deltaControl.getY().getValue();
                _cameraYaw -= deltaControl.getX().getValue();
                Vector3f cameraDirection = glm::eulerAngleYXZ(Math::radians(_cameraYaw), Math::radians(_cameraPitch), 0.f) * Vector4f(0.f, 0.f, 1.f, 1.f);
                getTransform()->setPosition(pivot + cameraDirection * _cameraDistance);
            } else if (mouse.getMouseRight().isPress()) {
                Vector2Control &deltaControl = mouse.getMouseDelta();
                Vector2f dir{ deltaControl.getX().getValue(), deltaControl.getY().getValue() };
                Vector2f norm{ 1.f, 0.f };
                if (Math::dot(dir, norm) > 0.f) {
                    _cameraDistance -= Math::length(dir) * 0.01f * _cameraDistance;
                } else {
                    _cameraDistance += Math::length(dir) * 0.01f * _cameraDistance;
                }

                Vector3f cameraDirection = glm::eulerAngleYXZ(Math::radians(_cameraYaw), Math::radians(_cameraPitch), 0.f) * Vector4f(0.f, 0.f, 1.f, 1.f);
                getTransform()->setPosition(pivot + cameraDirection * _cameraDistance);

            } else if (mouse.getMouseMiddle().isPress()) {
                Vector2Control &deltaControl = mouse.getMouseDelta();
                Vector2f dir{ deltaControl.getX().getValue(), deltaControl.getY().getValue() };
                Vector3f cameraUp = glm::eulerAngleYXZ(Math::radians(_cameraYaw), Math::radians(_cameraPitch), 0.f) * Vector4f(0.f, 1.f, 0.f, 1.f);
                Vector3f cameraRight = glm::eulerAngleYXZ(Math::radians(_cameraYaw), Math::radians(_cameraPitch), 0.f) * Vector4f(1.f, 0.f, 0.f, 1.f);

                pivot -= cameraUp * deltaControl.getY().getValue() * 0.01f * _cameraDistance;
                pivot -= cameraRight * deltaControl.getX().getValue() * 0.01f * _cameraDistance;

                Vector3f cameraDirection = glm::eulerAngleYXZ(Math::radians(_cameraYaw), Math::radians(_cameraPitch), 0.f) * Vector4f(0.f, 0.f, 1.f, 1.f);
                getTransform()->setPosition(pivot + cameraDirection * _cameraDistance);
            }
        }

        _cameraYaw = ClampAngle(_cameraYaw, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
        if (_cameraPitch > 180.f) {
            _cameraPitch = -(360.f - _cameraPitch);
        } else if (_cameraPitch < -180.f) {
            _cameraPitch = 360.f + _cameraPitch;
        }

        Quaternionf quaternion = glm::eulerAngleYXZ(Math::radians(_cameraYaw), Math::radians(_cameraPitch), 0.f);
        getTransform()->setRotation(quaternion);
    }

    void onLook(const Vector2f &value) {
        if (!disableMovement) {
            _inputLook = value;
            _inputLook.x = -_inputLook.x;
        }

        if (std::abs(value.x) > 0.01f || std::abs(value.y) > 0.01f) {

        }
    }

    void onMove(const Vector2f &value) {
        if (!disableMovement) {
            _inputMove = value;
        }
        if (std::abs(value.x) > 0.01f || std::abs(value.y) > 0.01f) {
            //            AN_LOG(Log, "move %f %f rotation %f", value.x, value.y, Math::degrees(rotation));
        }
    }
};

IMPLEMENT_AN_CLASS_HAS_INIT_ONLY(SceneBehavior)
LOAD_AN_CLASS(SceneBehavior)

SceneBehavior::~SceneBehavior() {}


ViewportPanel::ViewportPanel() : sceneTarget() {
    sceneTarget = MakeObjectPtr<RenderTarget>();
    
    RenderTargetDescriptor attachmentDescriptor{};
    attachmentDescriptor.width = 1920;
    attachmentDescriptor.height = 1080;
    attachmentDescriptor.samples = 1;
    attachmentDescriptor.format = kRTFormatRGBA16Float;

    SamplerDescriptor samplerDescriptor = Texture::DefaultSamplerDescriptor();
    samplerDescriptor.addressModeU = kSamplerAddressModeClampToEdge;
    samplerDescriptor.addressModeV = kSamplerAddressModeClampToEdge;
    samplerDescriptor.addressModeW = kSamplerAddressModeClampToEdge;

    ANAssert(sceneTarget->init(attachmentDescriptor, samplerDescriptor));

    Camera *camera = GetMainIMGUI().addComponent<Camera>();
    camera->setMatchLayerRatio(false);
    camera->setRenderTarget(sceneTarget.get());

    sceneBehavior = GetMainIMGUI().addComponent<SceneBehavior>();
    sceneBehavior->setScenePanel(this);
}

void ViewportPanel::onGUI() {
    {
        ImGui::Begin("Scene");

        bFocus = ImGui::IsWindowFocused();

        ImVec2 size = ImGui::GetContentRegionAvail();

        bool shouldAdjustSceneTarget = false;
        if (sceneTarget == nullptr) {
            shouldAdjustSceneTarget = true;
        }
        if (sceneTarget) {
            Size targetSize = sceneTarget->getSize();
            if (targetSize.width != (UInt32)size.x || targetSize.height != (UInt32)size.y) {
                shouldAdjustSceneTarget = true;
            }
        }

        Camera *camera = GetMainIMGUI().getComponent<Camera>();

        if (shouldAdjustSceneTarget) {
            if (camera) {
                camera->setViewportRatio(size.x / size.y);
            }
        }


        ImVec2 imageCursorPos  = ImGui::GetCursorPos();
        ImVec2 imageBlockBegin = imageCursorPos;
        ImGui::Image(sceneTarget.get(), size);

        if (ImGui::IsMouseReleased(0) && ImGui::IsItemHovered() && bFocus && !sceneBehavior->action && !ImGuizmo::IsUsing()) {
            /// handle pick renderer
            ImVec2 mousePosScreen  = ImGui::GetMousePos();
            ImVec2 windowPosScreen = ImGui::GetWindowPos();
            ImVec2 mousePosWindow  = mousePosScreen - windowPosScreen;
            ImVec2 cursorPosRel    = mousePosWindow - imageBlockBegin;

            ImVec2 imagePixelSize{(float) sceneTarget->getSize().width, (float) sceneTarget->getSize().height};

            ImVec2 pixelPos = cursorPosRel / size * imagePixelSize;
//            AN_LOG(Debug, "Pick %f %f", pixelPos.x, pixelPos.y);

            PickObject(*camera, { pixelPos.x, pixelPos.y });
        }

        if (Selection::GetActiveActor()) {

            bool snap = false;
            if (bFocus) {
                Keyboard &keyboard = GetInputManager().getKeyboard();
                if (keyboard.getButtonControl(kInputKey_W)->wasPressedThisFrame()) {
                    gizmoType = ImGuizmo::TRANSLATE;
                } else if (keyboard.getButtonControl(kInputKey_E)->wasPressedThisFrame()) {
                    gizmoType = ImGuizmo::ROTATE;
                } else if (keyboard.getButtonControl(kInputKey_R)->wasPressedThisFrame()) {
                    gizmoType = ImGuizmo::SCALE;
                }

                snap = keyboard.getButtonControl(kInputKeyLeftControl)->isPress();
            }

            float snapValue = 0.5f; // Snap to 0.5m for translation/scale
            // Snap to 45 degrees for rotation
            if (gizmoType == ImGuizmo::OPERATION::ROTATE)
                snapValue = 15.0f;

            float snapValues[3] = { snapValue, snapValue, snapValue };

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            ImVec2 wndPos = ImGui::GetWindowPos();
            ImVec2 rectOrigin = wndPos + imageCursorPos;
            ImGuizmo::SetRect(rectOrigin.x, rectOrigin.y, size.x, size.y);

            Matrix4x4f transform = Selection::GetActiveActor()->getTransform()->getLocalToWorldMatrix();
            ImGuizmo::Manipulate(Math::value_ptr(camera->getViewMatrix()), Math::value_ptr(camera->getProjectionMatrix()),
                                 gizmoType, ImGuizmo::WORLD, glm::value_ptr(transform),
                                 nullptr, snap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing() && !sceneBehavior->action) {
                Vector3f translation, eulerRad, scale;
                DecomposeTransform(transform, translation, eulerRad, scale);

                Quaternionf qua(eulerRad);

                Selection::GetActiveActor()->getTransform()->setPosition(translation);
                Selection::GetActiveActor()->getTransform()->setRotation(qua);
                Selection::GetActiveActor()->getTransform()->setLocalScale(scale);
            }
        }


        if (Event::Current().getType() == AN::kDragExited) {
            dragAndDropUpdating = false;
        }

        if (dragAndDropUpdating) {
            ImDrawList *drawList    = ImGui::GetWindowDrawList();
            ImVec2      startPos    = imageBlockBegin + ImGui::GetWindowPos();// Starting position of the rectangle
            ImVec2      endPos      = startPos + size;                   // Ending position of the rectangle
            ImU32       borderColor = IM_COL32(55, 142, 240, 255);            // Border color (red in this example)
            float       borderWidth = 2.0f;                                   // Border width in pixels

            drawList->AddRect(startPos, endPos, borderColor, 0.0f, ImDrawCornerFlags_All, borderWidth);
        }

        // Handle mouse input for dragging the image
        if (ImGui::IsItemHovered()) {

            bMouseHover = true;

            /// drag and drop
            if (Event::Current().getType() == AN::kDragUpdated) {
                if (GetDragAndDrop().getPaths().size() == 1) {
                    GetDragAndDrop().setVisualMode(AN::kDragOperationCopy);

                    dragAndDropUpdating = true;
                }

            } else if (Event::Current().getType() == kDragPerform) {
                if (GetDragAndDrop().getPaths().size() == 1) {
                    AN_LOG(Debug, "%s", GetDragAndDrop().getPaths()[0].c_str());
                }

                dragAndDropUpdating = false;
            }

        } else {
            bMouseHover = false;
            dragAndDropUpdating = false;
        }
        ImGui::End();
    }
}

}