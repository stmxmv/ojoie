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
#include <ojoie/Threads/Dispatch.hpp>
#include <ojoie/HAL/FileWatcher.hpp>
#include <ojoie/Serialize/SerializeManager.hpp>

#include <filesystem>

#undef SetCurrentDirectory


static const char *s_ErrorShaderCode = R"(Shader "Hidden/ErrorShader"
{
    Properties
    {
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque" "RenderPipeline" = "UniversalRenderPipeline" }

        HLSLINCLUDE
        #include "Core.hlsl"
        #include "Lighting.hlsl"
        ENDHLSL

        Pass
        {
            Tags { "LightMode" = "Forward" }

            HLSLPROGRAM

            struct appdata
            {
                float3 vertex : POSITION;
            };
            struct v2f
            {
                float4 vertexOut : SV_POSITION;
            };
            v2f vertex_main(appdata v)
            {
                v2f o;
                o.vertexOut = TransformWorldToHClip(v.vertex.xyz);
                return o;
            }
            half4 fragment_main(v2f i) : SV_TARGET
            {
                return half4(0.8, 0.0, 0.0 ,1.0);
            }
            ENDHLSL
        }
    }
}
)";

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


static FileWatcher s_FileWatcher;


void AppDelegate::applicationDidFinishLaunching(AN::Application *application) {
    //        MessageBoxW(nullptr, L"some text", L"caption", MB_OK);

    std::string projectRoot = application->getCommandLineArg<std::string>("--project-root");
    SetProjectRoot(projectRoot.c_str());
    SetCurrentDirectory(projectRoot.c_str());


    s_FileWatcher.onFileChange.bind([](const ChangeRecord &record) {
        Dispatch::async(Dispatch::Main, [record] {
            if (record.action == kFileModified) {
#ifdef AN_DEBUG
                AN_LOG(Debug, "fileWatcher %s", record.path.c_str());
#endif
//                return;
                std::filesystem::path path(record.path);
                if (path.extension() == ".shader") {
                    std::string shaderName = path.stem().string();
                    Object     *object     = GetResourceManager().getResourceExact(path.replace_extension("asset").string().c_str());
                    if (object) {
                        Shader *shader = object->as<Shader>();
                        /// set script path force recompile
                        if (shader->setScript(record.path)) {
                            shader->destroyGPUObject();
                            shader->createGPUObject();
                            AN_LOG(Debug, "recompile shader success");
                            auto assetPath = path.replace_extension("asset");
                            GetSerializeManager().serializeObjectAtPath(shader, assetPath.string().c_str());
                        } else {
                            /// set error script
                            ANAssert(shader->setScriptText(s_ErrorShaderCode));
                            shader->destroyGPUObject();
                            shader->createGPUObject();
                        }
                    }
                }
            }
        });
    });
    s_FileWatcher.start({ projectRoot }, kDefaultNotifyFlags);


    /// load all project assets
    for (auto &path : std::filesystem::recursive_directory_iterator(projectRoot)) {
        if (path.is_directory() || path.path().extension() != ".asset") { continue; }
        GetResourceManager().loadResourceExact(path.path().string().c_str());
    }

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

    s_FileWatcher.stop();
}

}