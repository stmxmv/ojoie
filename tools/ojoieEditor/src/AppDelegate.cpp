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

    InputAction &NavigationEnableAction = actionMap->addAction("NavigationEnable", AN::kInputControlButton);
    NavigationEnableAction
            .addBinding(kMouseRightButton)
            .addTrigger<PressedTrigger>();

    InputAction &NavigationDisableAction = actionMap->addAction("NavigationDisable", AN::kInputControlButton);
    NavigationDisableAction
            .addBinding(kMouseRightButton)
            .addTrigger<ReleaseTrigger>();

    InputAction &fullScreenAction = actionMap->addAction("ToggleFullScreen", AN::kInputControlButton);
    fullScreenAction
            .addBinding(kInputKey_F)
            .addTrigger<ModifierTrigger>(std::vector<int>{ kInputKeyLeftShift, kInputKeyLeftControl });

    InputAction &frameCaptureAction = actionMap->addAction("FrameCapture", AN::kInputControlButton);
    frameCaptureAction.addBinding(kInputKey_C);

    InputAction &sceneAction = actionMap->addAction("SceneActionEnable", kInputControlButton);
    sceneAction.addBinding(kInputKeyLeftAlt).addTrigger<PressedTrigger>();
    sceneAction.addBinding(kInputKeyRightAlt).addTrigger<PressedTrigger>();

    InputAction &sceneActionDis = actionMap->addAction("SceneActionDisable", kInputControlButton);
    sceneActionDis.addBinding(kInputKeyLeftAlt).addTrigger<ReleaseTrigger>();
    sceneActionDis.addBinding(kInputKeyRightAlt).addTrigger<ReleaseTrigger>();

    InputAction &sceneFocusAction = actionMap->addAction("SceneFocus", kInputControlButton);
    sceneFocusAction.addBinding(kInputKey_F);

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


//    createCubeActor();

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