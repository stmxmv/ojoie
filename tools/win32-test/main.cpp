//
// Created by aojoie on 4/22/2023.
//


#define IMGUI_DEFINE_MATH_OPERATORS

#include <iostream>
#include <ojoie/Core/App.hpp>
#include <ojoie/Core/Window.hpp>
#include <ojoie/Core/Screen.hpp>
#include <ojoie/Core/Game.hpp>
#include <ojoie/Threads/Dispatch.hpp>
#include <ojoie/Camera/Camera.hpp>
#include <ojoie/Core/Configuration.hpp>

#include <ojoie/Render/TextureLoader.hpp>

#include <ojoie/Render/Mesh/MeshRenderer.hpp>
#include <ojoie/Asset/FBXImporter.hpp>

#include <ojoie/Serialize/Coder/YamlDecoder.hpp>
#include <ojoie/IO/FileInputStream.hpp>
#include <ojoie/Core/Behavior.hpp>
#include <ojoie/Input/InputComponent.hpp>
#include <ojoie/Render/QualitySettings.hpp>

#include <ojoie/Core/Event.hpp>
#include <ojoie/Core/DragAndDrop.hpp>
#include <ojoie/IMGUI/IMGUI.hpp>

#include <ojoie/Misc/ResourceManager.hpp>

#include <ojoie/Export/Script.h>
#include <ojoie/Export/mono/mono.h>

#include <Windows.h>

using std::cout, std::endl;

#include <Windows.h>
#include <wrl\client.h>

using Microsoft::WRL::ComPtr;
using namespace AN;

AN::Window *gMainWindow;

class IMGUIDemo : public IMGUI {

    Texture2D *tex;

    bool dragAndDropUpdating = false;

    AN_CLASS(IMGUIDemo, IMGUI)

public:

    virtual bool init() override {
        if (!Super::init()) return false;

        tex = NewObject<Texture2D>();
        TextureDescriptor descriptor{};
        descriptor.width = 255;
        descriptor.height = 255;
        descriptor.mipmapLevel = 1;
        descriptor.pixelFormat = kPixelFormatRGBA8Unorm_sRGB;

        std::vector<UInt8> data(255 * 255 * 4, 255); // set a white image

        SamplerDescriptor samplerDescriptor = Texture::DefaultSamplerDescriptor();
        samplerDescriptor.filter = AN::kSamplerFilterNearest;

        if (!tex->init(descriptor, samplerDescriptor)) return false;
        tex->setPixelData(data.data());
        tex->setReadable(true);
        tex->uploadToGPU(false);

        return true;
    }

    using IMGUI::IMGUI;

    virtual void onGUI() override {


        ImGuiIO& io = ImGui::GetIO();

        {
            ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::SetNextWindowBgAlpha(0.0f);

            ImGuiWindowFlags host_window_flags = 0;
            host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
            host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
//            host_window_flags |= ImGuiWindowFlags_MenuBar;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("MainDockSpaceViewport", nullptr, host_window_flags);
            ImGui::PopStyleVar(3);

            ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

            ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        }
        static bool show_demo_window = true, show_another_window = true;
        static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f * GetGame().deltaTime, 1.f / GetGame().deltaTime);
            ImGui::End();
        }

        {
            ImGui::Begin("ANFridgeGUI");
            if (ImGui::Button("OpenFile")) {
                OpenPanel *openPanel = OpenPanel::Alloc();

                if (!openPanel->init()) {
                    AN_LOG(Error, "init openPannel fail");
                } else {
                    openPanel->setAllowOtherTypes(true);
                    openPanel->addAllowContentExtension("JPG Image", "jpg");
                    openPanel->addAllowContentExtension("PNG Image", "png");
                    openPanel->setDefaultExtension("png");

                    openPanel->beginSheetModal(gMainWindow, [this](ModalResponse res, const char *path) {
                        if (res == AN::kModalResponseOk && path) {
                            AN_LOG(Log, "Load image %s", path);
                            auto result = TextureLoader::LoadTexture(path);
                            if (result.isValid()) {
                                tex->resize(result.getWidth(), result.getHeight());
                                tex->setPixelData(result.getData());
                                tex->uploadToGPU(false);
                            }
                        }
                    });
                }
                openPanel->release();
            }

            ImVec2 size = ImGui::GetContentRegionAvail();

            float imageRatio = (float)tex->getDataWidth() / (float)tex->getDataHeight();
            float imageHeight = size.y;
            float imageWidth = imageRatio * imageHeight;
            if (imageWidth > size.x) {
                imageWidth = size.x;
                imageHeight = imageWidth / imageRatio;
            }

            ImVec2 imageSize{ imageWidth, imageHeight };
            ImVec2 imageCursorPos = ImGui::GetCursorPos();
            ImVec2 imageBlockBegin = imageCursorPos + (size - imageSize) * 0.5f;
            ImGui::SetCursorPos(imageBlockBegin);
            ImGui::Image(tex, imageSize);

            if (Event::Current().getType() == AN::kDragExited) {
                dragAndDropUpdating = false;
            }

            if (dragAndDropUpdating) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 startPos = imageBlockBegin + ImGui::GetWindowPos(); // Starting position of the rectangle
                ImVec2 endPos = startPos + imageSize; // Ending position of the rectangle
                ImU32 borderColor = IM_COL32(55, 142, 240, 255); // Border color (red in this example)
                float borderWidth = 2.0f; // Border width in pixels

                drawList->AddRect(startPos, endPos, borderColor, 0.0f, ImDrawCornerFlags_All, borderWidth);
            }

            // Handle mouse input for dragging the image
            if (ImGui::IsItemHovered()) {
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
                dragAndDropUpdating = false;
            }
            ImGui::End();

            ImGui::End();// MainDockSpace
        }
    }
};

IMPLEMENT_AN_CLASS(IMGUIDemo)
LOAD_AN_CLASS(IMGUIDemo)

IMGUIDemo::~IMGUIDemo() {}

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
        gMainWindow->setFullScreen(!gMainWindow->isFullScreen());
    }

    AN_CLASS(MainBehaviour, Behavior)

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

IMPLEMENT_AN_CLASS_INIT(MainBehaviour)
LOAD_AN_CLASS(MainBehaviour)

MainBehaviour::~MainBehaviour() {}

class AppDelegate : public AN::ApplicationDelegate {

    AN::RefCountedPtr<AN::Window> mainWindow;
    AN::RefCountedPtr<AN::Menu> mainMenu;
    AN::RefCountedPtr<AN::Menu> appMenu;
    AN::Size size;
public:
    virtual void applicationWillFinishLaunching(Application *application) override {
        application->setDarkMode(kDarkMode);
    }

    virtual bool applicationShouldTerminateAfterLastWindowClosed(AN::Application *application) override {
        return true;
    }

    virtual void applicationDidFinishLaunching(AN::Application *application) override {

        //        MessageBoxW(nullptr, L"some text", L"caption", MB_OK);

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
        mainWindow->setTitle("win32-test 有没有");
        mainWindow->center();
        mainWindow->makeKeyAndOrderFront();

        gMainWindow = mainWindow.get();

        //        AN::Cursor::setState(AN::kCursorStateDisabled);
        //        mainWindow->setCursorState(AN::kCursorStateDisabled);
        ANAssert(AN::App->getMainWindow() == mainWindow.get());
    }

    virtual void applicationWillTerminate(AN::Application *application) override {
        /// release resources here
        mainWindow = nullptr;

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
        return actionMap;
    }

    Actor *createCubeActor() {
        Actor *cubeActor = NewObject<Actor>();
        cubeActor->init("Cube");

        File file;
        FileInputStream fileInputStream(file);
        file.open("Data/Assets/BusterDrone.asset", kFilePermissionRead);
        YamlDecoder meshDecoder(fileInputStream);
        Mesh *mesh = NewObject<Mesh>();
        mesh->redirectTransferVirtual(meshDecoder);
        ANAssert(mesh->initAfterDecode());

        mesh->createVertexBuffer();

        MeshRenderer *meshRenderer = cubeActor->addComponent<MeshRenderer>();
        meshRenderer->setMesh(mesh);

        Shader *shader = NewObject<Shader>();
        file.open("Data/Assets/DefaultShader.asset", AN::kFilePermissionRead);
        YamlDecoder decoder(fileInputStream);
        shader->redirectTransferVirtual(decoder);

        ANAssert(shader->initAfterDecode());
        ANAssert(shader->createGPUObject());

        Material *material = NewObject<Material>();
        material->init(shader, "DefaultMaterial");

        file.open("Data/Assets/BusterDroneTex.asset", kFilePermissionRead);
        YamlDecoder texDecoder(fileInputStream);
        Texture2D *texture = NewObject<Texture2D>();
        texture->redirectTransferVirtual(texDecoder);
        ANAssert(texture->initAfterDecode());
        texture->uploadToGPU(true);

        material->setTexture("_DiffuseTex", texture);

        {
            file.open("Data/Assets/BusterDroneNorm.asset", kFilePermissionRead);
            YamlDecoder normalMapDecoder(fileInputStream);
            Texture2D *NormalMap = NewObject<Texture2D>();
            NormalMap->redirectTransferVirtual(normalMapDecoder);
            ANAssert(NormalMap->initAfterDecode());
            NormalMap->uploadToGPU(false);

            material->setTexture("_NormalMap", NormalMap);
        }

        meshRenderer->setMaterial(0, material);

        return cubeActor;
    }

    virtual void gameSetup(AN::Game &game) override {

        Material::SetVectorGlobal("_GlossyEnvironmentColor", { 0.1f, 0.1f, 0.1f, 1.f });
        Material::SetVectorGlobal("_MainLightPosition", { 10.f, 10.f, 10.f, 1.f });
        Material::SetVectorGlobal("_MainLightColor", { 1.f, 1.f, 1.f, 1.f });
        Material::SetIntGlobal("_MainLightLayerMask", 0x1);
        Material::SetVectorGlobal("an_LightData", { 1.f, 1.f, 1.f, 1.f });

        int refleshRate = AN::GetScreen().getRefreshRate() * 2;
        game.setMaxFrameRate(refleshRate);
        //
        ////        Cursor::setState(AN::kCursorStateDisabled);
        //
        Actor *camera = NewObject<Actor>();
        camera->init("MainCamera");
        Camera *cameraCom = camera->addComponent<Camera>();
        cameraCom->setViewportRatio((float)size.width / size.height);

        InputActionMap *actionMap = createInputActionMap();
        InputComponent *inputComponent = camera->addComponent<InputComponent>();
        inputComponent->setActionMap(actionMap);

        camera->addComponent<MainBehaviour>();


        //        Actor *cubeActor = createCubeActor();
        //        cubeActor->getTransform()->setPosition({ 0.f, 0.f, -10.f });

        camera->addComponent<IMGUIDemo>();
    }
};


AN_API extern MonoDomain *gMonoDomain;
AN_API extern MonoImage *gOjoieImage;
AN_API MonoObject *GetSharedApplication_Injected();

class MonoAppDelegate : public AN::ApplicationDelegate {

    MonoAssembly *assembly;
    MonoImage *image;
    MonoClass *appDelegateClass;
    MonoObject *appDelegate;
    uint32_t gcHandle;

public:

    MonoAppDelegate() {
        assembly = mono_domain_assembly_open(gMonoDomain, "Data/Mono/MonoAssembly.dll");
        image = mono_assembly_get_image(assembly);

        MonoMethod *method = mono_class_get_method_from_name(mono_object_get_class(GetSharedApplication_Injected()), "GetTypesWithApplicationMainAttribute", 1);

        void *args[1] = { mono_assembly_get_object(gMonoDomain, assembly) };
        MonoReflectionType *refType = (MonoReflectionType *)mono_runtime_invoke(method, nullptr, args, nullptr);
        if (refType == nullptr) {
            AN_LOG(Error, "Not Found ApplicationMain");
            exit(-1);
        }

        MonoType *type = mono_reflection_type_get_type(refType);

        appDelegateClass = mono_type_get_class(type);
        appDelegate = mono_object_new(gMonoDomain, appDelegateClass);
        gcHandle = mono_gchandle_new(appDelegate, true);

        MonoProperty *prop = mono_class_get_property_from_name(mono_object_get_class(GetSharedApplication_Injected()), "AppDelegate");
        args[0] = { appDelegate };
        mono_property_set_value(prop, GetSharedApplication_Injected(), args, nullptr);
    }

    virtual void applicationWillFinishLaunching(Application *application) override {
        MonoMethod *method = mono_class_get_method_from_name(appDelegateClass, "ApplicationWillFinishLaunching", 1);
        if (method == nullptr) return;
        MonoObject *app = GetSharedApplication_Injected();
        void *args[1] = { app };
        mono_runtime_invoke(method, appDelegate, args, nullptr);
    }

    virtual void applicationDidFinishLaunching(Application *application) override {
        MonoMethod *method = mono_class_get_method_from_name(appDelegateClass, "ApplicationDidFinishLaunching", 1);
        if (method == nullptr) return;
        MonoObject *app = GetSharedApplication_Injected();
        void *args[1] = { app };
        mono_runtime_invoke(method, appDelegate, args, nullptr);
    }

    virtual void applicationWillTerminate(Application *application) override {
        MonoMethod *method = mono_class_get_method_from_name(appDelegateClass, "ApplicationWillTerminate", 1);
        if (method == nullptr) {
            mono_gchandle_free(gcHandle);
            return;
        }
        MonoObject *app = GetSharedApplication_Injected();
        void *args[1] = { app };
        mono_runtime_invoke(method, appDelegate, args, nullptr);

        mono_gchandle_free(gcHandle);
    }

    virtual bool applicationShouldTerminateAfterLastWindowClosed(Application *application) override {
        MonoMethod *method = mono_class_get_method_from_name(appDelegateClass, "ApplicationShouldTerminateAfterLastWindowClosed", 1);
        if (method == nullptr) {
            return false;
        }
        MonoObject *app = GetSharedApplication_Injected();
        void *args[1] = { app };
        MonoObject *ret = mono_runtime_invoke(method, appDelegate, args, nullptr);

        bool shouldClose = !!*(MonoBoolean *) mono_object_unbox (ret);
        return shouldClose;
    }

    virtual void gameSetup(Game &game) override {
        MonoMethod *method = mono_class_get_method_from_name(appDelegateClass, "Setup", 0);
        if (method == nullptr) {
            return;
        }
        mono_runtime_invoke(method, appDelegate, nullptr, nullptr);
    }

    virtual void gameStart(Game &game) override {
        MonoMethod *method = mono_class_get_method_from_name(appDelegateClass, "Start", 0);
        if (method == nullptr) {
            return;
        }
        mono_runtime_invoke(method, appDelegate, nullptr, nullptr);
    }

    virtual void gameStop(Game &game) override {
        MonoMethod *method = mono_class_get_method_from_name(appDelegateClass, "Stop", 0);
        if (method == nullptr) {
            return;
        }
        mono_runtime_invoke(method, appDelegate, nullptr, nullptr);
    }
};


int main(int argc, const char *argv[]) {
    ScriptRuntimeInit();

    AN::Application &app = AN::Application::GetSharedApplication();
    MonoObject *monoApp = GetSharedApplication_Injected();

    MonoAppDelegate *appDelegate = new MonoAppDelegate();
    app.setDelegate(appDelegate);
    appDelegate->release();

    //    try {
    app.run(argc, argv);

    ScriptRuntimeCleanup();

    //    } catch (const std::exception &exception) {
    //
    //#ifdef _WIN32
    //        MessageBoxA(nullptr, exception.what(), "Exception", MB_OK|MB_ICONERROR);
    //#endif
    //
    //    }

    return 0;
}