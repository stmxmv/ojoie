//
// Created by aojoie on 6/24/2023.
//

#include "AppDelegate.hpp"
#include "MainIMGUI.hpp"

#include "Project/Project.hpp"

#include <ojoie/App/Views/ImageView.hpp>
#include <ojoie/Core/Screen.hpp>
#include <ojoie/Render/QualitySettings.hpp>
#include <ojoie/Render/Material.hpp>
#include <ojoie/Core/Game.hpp>
#include <ojoie/Camera/Camera.hpp>
#include <ojoie/Input/InputManager.hpp>
#include <ojoie/Core/Behavior.hpp>
#include <ojoie/Render/RenderManager.hpp>
#include <ojoie/HAL/File.hpp>
#include <ojoie/Render/Mesh/Mesh.hpp>
#include <ojoie/Misc/ResourceManager.hpp>
#include <ojoie/Render/Mesh/MeshRenderer.hpp>

namespace AN::Editor {

class MainBehaviour : public Behavior {

    float _cameraYaw; // in degrees
    float _cameraPitch = 0.f;

    Vector2f _inputMove{};
    Vector2f _inputLook{};

    bool disableMovement = true;


    static float ClampAngle(float lfAngle, float lfMin, float lfMax) {
        if (lfAngle < -360.f) lfAngle += 360.f;
        if (lfAngle > 360.f) lfAngle -= 360.f;
        return std::clamp(lfAngle, lfMin, lfMax);
    }

    static void OnMoveMessage(void *receiver, Message &message) {
        MainBehaviour *self = (MainBehaviour *) receiver;
        self->onMove(*message.getData<Vector2f *>());
    }

    static void OnLookMessage(void *receiver, Message &message) {
        MainBehaviour *self = (MainBehaviour *) receiver;
        self->onLook(*message.getData<Vector2f *>());
    }

    static void OnTerminateMessage(void *receiver, Message &message) {
        App->terminate();
    }

    static void OnCursorStateMessage(void *receiver, Message &message) {
        MainBehaviour *self = (MainBehaviour *)receiver;
        if (Cursor::getState() == kCursorStateDisabled) {
            AN::Cursor::setState(AN::kCursorStateNormal);
            self->disableMovement = true;
        } else {
            AN::Cursor::setState(AN::kCursorStateDisabled);
            self->disableMovement = false;
        }
    }

    static void OnToggleFullScreen(void *receiver, Message &message) {
        GetMainWindow()->setFullScreen(!GetMainWindow()->isFullScreen());
    }

    static void OnFrameCaptureMessage(void *receiver, Message &message) {
        GetRenderManager().captureNextFrame();
    }

    DECLARE_DERIVED_AN_CLASS(MainBehaviour, Behavior)

public:

    /// [Tooltip("How far in degrees can you move the camera up")]
    float topClamp = 90.0f;

    /// [Tooltip("How far in degrees can you move the camera down")]
    float bottomClamp = -90.0f;

    explicit MainBehaviour(ObjectCreationMode mode) : Super(mode) {}


    static void InitializeClass() {
        GetClassStatic()->registerMessageCallback("OnTerminate", OnTerminateMessage);
        GetClassStatic()->registerMessageCallback("OnMove", OnMoveMessage);
        GetClassStatic()->registerMessageCallback("OnLook", OnLookMessage);
        GetClassStatic()->registerMessageCallback("OnCursorState", OnCursorStateMessage);
        GetClassStatic()->registerMessageCallback("OnToggleFullScreen", OnToggleFullScreen);
        GetClassStatic()->registerMessageCallback("OnFrameCapture", OnFrameCaptureMessage);
    }

    virtual void start() override {
        Super::start();
        _cameraYaw = getTransform()->getEulerAngles().y;
    }

    virtual void update() override {
        Super::update();

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

            Vector3f position = getTransform()->getPosition();
            position += targetDirection * GetGame().deltaTime * 10.f;
            getTransform()->setPosition(position);

            _inputMove = {};
        }


        if (std::abs(_inputLook.x) > 0.01f || std::abs(_inputLook.y) > 0.01f) {
            //Don't multiply mouse input by Time.deltaTime
            float multiplier = 1.f;
            _cameraYaw += _inputLook.x * multiplier;
            _cameraPitch += _inputLook.y * multiplier;

            _inputLook = {};
        }

        _cameraYaw = ClampAngle(_cameraYaw, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
        _cameraPitch = ClampAngle(_cameraPitch, bottomClamp, topClamp);

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

IMPLEMENT_AN_CLASS_HAS_INIT_ONLY(MainBehaviour)
LOAD_AN_CLASS(MainBehaviour)

MainBehaviour::~MainBehaviour() {}



void AppDelegate::applicationWillFinishLaunching(Application *application) {
    application->setDarkMode(kDarkMode);

    ImageView *imageView = ImageView::Alloc();
    ANAssert(imageView->init("SPLASH_IMAGE1"));

    splashWindow = Window::Alloc();

    ANAssert(splashWindow->init(imageView->getFrame(), false));
    splashWindow->setBorderLessStyle();
    splashWindow->addSubView(imageView);
    splashWindow->center();
    splashWindow->makeKeyAndOrderFront();

    imageView->release();

    splashTimer.mark();
    splashTimer.elapsedTime = 0.f;
}

void AppDelegate::applicationDidFinishLaunching(AN::Application *application) {
    //        MessageBoxW(nullptr, L"some text", L"caption", MB_OK);

    std::string projectRoot = application->getCommandLineArg<std::string>("--project-root");
    SetProjectRoot(projectRoot.c_str());
    SetCurrentDirectory(projectRoot.c_str());

    class AboutMenuDelegate : public MenuInterface {
    public:
        virtual void execute(const MenuItem &menuItem) override {
            //                MessageBoxW(nullptr, L"about content", L"About", MB_OK);
            App->showAboutWindow();
        }
    };

    class CloseMenuDelegate : public MenuInterface {
    public:
        virtual void execute(const MenuItem &menuItem) override {
            App->terminate();
        }
    };
    AboutMenuDelegate *about = new AboutMenuDelegate();
    CloseMenuDelegate *close = new CloseMenuDelegate();

    appMenu = ref_transfer(Menu::Alloc());
    ANAssert(appMenu->init(AN::kMenuPopup));

    appMenu->addMenuItem("About #%a", "About", about);
    appMenu->addMenuItem("Close #%q", "Close", close);
    about->release();
    close->release();

    mainMenu = ref_transfer(Menu::Alloc());
    ANAssert(mainMenu->init(AN::kMenuTopLevel));
    mainMenu->addSubMenu("App", appMenu.get());


    size = AN::GetScreen().getSize();

    /// set the render target size as screen size no matter what window size is
    /// otherwise it will automatically set to window size, and it will not automatically change when you resize window,
    /// it may require recreate the whole framebuffer which causes large overhead instead it will only recreate
    /// the swapchain image
    GetQualitySettings().setTargetResolution(size);

    mainWindow = AN::ref_transfer(AN::Window::Alloc());
    //        AN::Size screenSize = AN::GetDefaultScreenSize();
    mainWindow->init({0, 0, size.width * 4 / 5, size.height * 4 / 5 });
    mainWindow->setMenu(mainMenu.get());
    mainWindow->setTitle("ojoie editor <DX11>");
    mainWindow->center();


    //        AN::Cursor::setState(AN::kCursorStateDisabled);
    //        mainWindow->setCursorState(AN::kCursorStateDisabled);
    ANAssert(AN::App->getMainWindow() == mainWindow.get());
}


InputActionMap *createInputActionMap() {
    InputActionMap *actionMap = NewObject<InputActionMap>();
    actionMap->init("MainInput");
    InputAction &action = actionMap->addAction("Move", kInputControlVector2);
    action.addVector2CompositeBinding(kInputKey_W, kInputKey_S, kInputKey_A, kInputKey_D);

    /// when add another action, above action may become invalid pointer
    InputAction &lookAction = actionMap->addAction("Look", kInputControlVector2);
    lookAction.addBinding(kPointerDelta);

    InputAction &terminateAction = actionMap->addAction("Terminate", AN::kInputControlButton);
    terminateAction.addBinding(kInputKeyEsc);

    InputAction &cursorAction = actionMap->addAction("CursorState", AN::kInputControlButton);
    cursorAction.addBinding(kInputKey_K);

    InputAction &fullScreenAction = actionMap->addAction("ToggleFullScreen", AN::kInputControlButton);
    fullScreenAction.addBinding(kInputKey_F);

    InputAction &frameCaptureAction = actionMap->addAction("FrameCapture", AN::kInputControlButton);
    frameCaptureAction.addBinding(kInputKey_C);

    return actionMap;
}

Actor *createCubeActor() {
    Actor *cubeActor = NewObject<Actor>();
    cubeActor->init("TestActor");

    Mesh *mesh = (Mesh *)GetResourceManager().loadResource(Mesh::GetClassNameStatic(), "BusterDrone", GetApplicationFolder().c_str());

    mesh->createVertexBuffer();

    MeshRenderer *meshRenderer = cubeActor->addComponent<MeshRenderer>();
    meshRenderer->setMesh(mesh);

    Shader *shader = (Shader *)GetResourceManager().getResource(Shader::GetClassNameStatic(), "test");

    Material *material = NewObject<Material>();
    material->init(shader, "DefaultMaterial");

    Texture2D *texture = (Texture2D *)GetResourceManager().loadResource(Texture2D::GetClassNameStatic(), "BusterDroneTex", GetApplicationFolder().c_str());
    material->setTexture("_DiffuseTex", texture);

    {
        Texture2D *NormalMap = (Texture2D *)GetResourceManager().loadResource(Texture2D::GetClassNameStatic(), "BusterDroneNorm", GetApplicationFolder().c_str());
        material->setTexture("_NormalMap", NormalMap);
    }

    meshRenderer->setMaterial(0, material);

    return cubeActor;
}

void AppDelegate::gameSetup(AN::Game &game) {
    Material::SetVectorGlobal("_GlossyEnvironmentColor", { 0.1f, 0.1f, 0.1f, 1.f });
    Material::SetVectorGlobal("_MainLightPosition", { 10.f, 10.f, 10.f, 1.f });
    Material::SetVectorGlobal("_MainLightColor", { 1.f, 1.f, 1.f, 1.f });
    Material::SetIntGlobal("_MainLightLayerMask", 0x1);
    Material::SetVectorGlobal("an_LightData", { 1.f, 1.f, 1.f, 1.f });

    int refleshRate = AN::GetScreen().getRefreshRate() * 2;
    game.setMaxFrameRate(60);
}

void AppDelegate::gameStart(Game &game) {

    Actor *editorManager = NewObject<Actor>();
    editorManager->init("EditorManager");

    InputActionMap *actionMap = createInputActionMap();
    InputComponent *inputComponent = editorManager->addComponent<InputComponent>();
    inputComponent->setActionMap(actionMap);

    editorManager->addComponent<MainIMGUI>();
    editorManager->addComponent<MainBehaviour>();


    createCubeActor();

    float timeToSleep;
    for (;;) {
        splashTimer.mark();
        /// assure splash window show more than 4 secs
        timeToSleep = 3.f - splashTimer.elapsedTime;

        if (timeToSleep < 0.f) {
            break;
        }

        App->pollEvent();

        cpu_relax();
        cpu_relax();
        cpu_relax();
        std::this_thread::yield();
    }

    splashWindow->release();

    //    mainWindow->zoom(); /// maximum the window
    mainWindow->makeKeyAndOrderFront();
}

void AppDelegate::applicationWillTerminate(AN::Application *application) {
    mainWindow = nullptr;
    mainMenu = nullptr;
    appMenu = nullptr;
}

}