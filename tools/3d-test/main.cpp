//
// Created by Aleudillonam on 7/26/2022.
//

#if defined(VLD_MEM_CHECK)
#include <vld.h>
#endif

#include <imgui/imgui.h>
#include <ojoie/Audio/FlacFile.hpp>
#include <ojoie/Audio/Mp3File.hpp>
#include <ojoie/Audio/Sound.hpp>
#include <ojoie/Audio/WavFile.hpp>
#include <ojoie/Core/Configuration.hpp>
#include <ojoie/Core/Dispatch.hpp>
#include <ojoie/Core/Game.hpp>
#include <ojoie/Core/Log.h>
#include <ojoie/Core/Node.hpp>
#include <ojoie/Core/Task.hpp>
#include <ojoie/Core/Window.hpp>
#include <ojoie/Input/InputManager.hpp>
#include <ojoie/Node/CameraNode.hpp>
#include <ojoie/Node/StaticMeshNode.hpp>
#include <ojoie/Node/StaticModelNode.hpp>
#include <ojoie/Node/TextNode.hpp>
#include <ojoie/Render/Font.hpp>
#include <ojoie/UI/ImguiNode.hpp>

#include <ojoie/Render/RenderQueue.hpp>

#include <ojoie/Geometry/Sphere.hpp>

#include <ojoie/Render/TextureLoader.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <filesystem>
#include <ranges>
#include <algorithm>
#include <future>

#ifdef _WIN32
#include <Windows.h>
#endif


bool insensitiveEqual(const std::string_view& lhs, const std::string_view& rhs) {
    auto to_lower{ std::ranges::views::transform(tolower) };
    return std::ranges::equal(lhs | to_lower, rhs | to_lower);
}

bool floatEqual(float a, float b) {
    constexpr static float epsilon = 0.001f;
    return std::abs(a - b) < epsilon;
}


using std::cin, std::cout, std::endl;

// clang-format off
static AN::Vertex cube_vertices[24] = {
        { { -0.5f, -0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f }},
        { { 0.5f,  0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f }},
        { { 0.5f, -0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f }},
        { { -0.5f,  0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f }},


        { { -0.5f, -0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f }},
        { {  0.5f, -0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f }, {1.0f, 1.0f }},
        { {  0.5f,  0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f }, {1.0f, 0.0f }},
        { { -0.5f,  0.5f,  0.5f }, { 0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f }},


        { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f }},
        { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f }},
        { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f }},
        { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f }},

        { {  0.5f,  0.5f,  0.5f }, { 1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f }},
        { {  0.5f, -0.5f, -0.5f }, { 1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f }},
        { {  0.5f,  0.5f, -0.5f }, { 1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f }},
        { {  0.5f, -0.5f,  0.5f }, { 1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f }},


        { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f }},
        { {  0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f }},
        { {  0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f }},
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f }},

        { { -0.5f,  0.5f, -0.5f }, { 0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f }},
        { {  0.5f,  0.5f,  0.5f }, { 0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f }},
        { {  0.5f,  0.5f, -0.5f }, { 0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f }},
        { { -0.5f,  0.5f,  0.5f }, { 0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f }},

};

static uint32_t cube_indices[36] = {
    0, 1, 2, 1, 0, 3,
    4, 5, 6, 6, 7, 4,
    8, 9, 10, 10, 11, 8,
    12, 13, 14, 13, 12, 15,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 21, 20, 23
};

//static AN::Vertex cube_vertices[] = {
//        { { -1.f, -1.f, -1.f }, { -1.f, -1.f, -1.f }, { 1.0f, 1.0f } },
//        { {  1.f, -1.f, -1.f }, { 1.f,  -1.f, -1.f }, { 0.0f, 1.0f } },
//        { {  1.f,  1.f, -1.f }, {  1.f,  1.f, -1.f }, { 0.0f, 0.0f } },
//        { { -1.f,  1.f, -1.f }, { -1.f,  1.f, -1.f }, { 1.0f, 0.0f } },
//        { { -1.f, -1.f,  1.f }, { -1.f, -1.f,  1.f }, { 0.0f, 0.0f } },
//        { {  1.f, -1.f,  1.f }, {  1.f, -1.f,  1.f }, { 1.0f, 0.0f } },
//        { {  1.f,  1.f,  1.f }, {  1.f,  1.f,  1.f }, { 1.0f, 1.0f } },
//        { { -1.f,  1.f,  1.f }, { -1.f,  1.f,  1.f }, { 0.0f, 1.0f } },
//};

//static unsigned int cube_indices[] = {
//        1, 0, 3, 1, 3, 2,
//        1, 2, 6, 1, 6, 5,
//        5, 6, 7, 5, 7, 4,
//        0, 4, 7, 0, 7, 3,
//        7, 6, 2, 7, 2, 3,
//        0, 5, 4, 0, 1, 5
//};

// clang-format on

class TransformEdit : public std::enable_shared_from_this<TransformEdit> {
    typedef TransformEdit Self;

    std::weak_ptr<AN::StaticMeshNode> _mesh;

    bool didSetPosition{};
    bool didFinishSetPosition{};
    AN::Math::vec3 position{};
    AN::Math::vec3 position_min{ -10.f, -10.f, -10.f };
    AN::Math::vec3 position_max{ 10.f, 10.f, 10.f };

    bool didSetRotation{};
    AN::Math::rotator rotation{};

    bool didSetScale{};
    bool didFinishSetScale{};
    AN::Math::vec3 scale{};
    AN::Math::vec3 scale_max{ 2.f, 2.f, 2.f };

    int revertButtonId;

    bool revertButton() {
        ImGui::PushID(std::format("{}_{}", (uint64_t)this, revertButtonId++).c_str());
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
        bool ret = ImGui::Button("Revert");
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        return ret;
    }

public:

    TransformEdit() = default;

    TransformEdit(const std::shared_ptr<AN::StaticMeshNode> &mesh)
        : _mesh(mesh) {}

    void setMesh(const std::shared_ptr<AN::StaticMeshNode> &mesh) {
        _mesh = mesh;
    }

    void update() {
        std::shared_ptr<AN::StaticMeshNode> mesh = _mesh.lock();
        if (mesh) {
            auto *meshSceneProxy = (AN::StaticMeshNode::SceneProxyType *)mesh->getSceneProxy();
            position = meshSceneProxy->transform.translation;
            rotation = meshSceneProxy->transform.rotator();
            scale    = meshSceneProxy->transform.scale;
        }
    }

    void draw() {
        revertButtonId = 0;
        ImGui::PushID(this);
        static const char *position_str = "P";
        ImGui::Text("%s", position_str);
        ImGui::SameLine();
        ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(position_str).x) * 0.25f );
        if (ImGui::DragFloat("X##position", &position.x, 0.1f, position_min.x, position_max.x)) {
            didSetPosition = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetPosition = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Y##position", &position.y, 0.1f, position_min.y, position_max.y)) {
            didSetPosition = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetPosition = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Z##position", &position.z, 0.1f, position_min.z, position_max.z)) {
            didSetPosition = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetPosition = true;
        }
        ImGui::SameLine();
        if (revertButton()) {
            didSetPosition = true;
            didFinishSetPosition = true;
            position = {};
        }
        ImGui::PopItemWidth();
        //            ImGui::EndChild();


        static const char *rotation_str = "R";
        ImGui::Text("%s", rotation_str);
        ImGui::SameLine();
        ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(rotation_str).x) * 0.25f );
        if (ImGui::DragFloat("P", &rotation.pitch, 1.f, -180.f, 180.f)) {
            didSetRotation = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Y", &rotation.yaw, 1.f, -180.f, 180.f)) {
            didSetRotation = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("R", &rotation.roll, 1.f, -180.f, 180.f)) {
            didSetRotation = true;
        }
        ImGui::SameLine();
        if (revertButton()) {
            didSetRotation = true;
            rotation = {};
        }
        ImGui::PopItemWidth();



        static const char *scale_str = "S";
        ImGui::Text("%s", scale_str);
        ImGui::SameLine();
        ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::CalcTextSize(scale_str).x) * 0.25f );
        if (ImGui::DragFloat("X##scale", &scale.x, 0.1f, 0.f, scale_max.x)) {
            didSetScale = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetScale = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Y##scale", &scale.y, 0.1f, 0.f, scale_max.y)) {
            didSetScale = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetScale = true;
        }
        ImGui::SameLine();
        if (ImGui::DragFloat("Z##scale", &scale.z, 0.1f, 0.f, scale_max.z)) {
            didSetScale = true;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            didFinishSetScale = true;
        }
        ImGui::SameLine();
        if (revertButton()) {
            didSetScale = true;
            didFinishSetScale = true;
            scale = { 1.f, 1.f, 1.f };
        }

        ImGui::PopItemWidth();

        ImGui::PopID();


        if (didSetPosition) {
            didSetPosition = false;
            AN::Dispatch::async(AN::Dispatch::Game, [position = position, _mesh = _mesh] {
                auto mesh = _mesh.lock();
                if (mesh) {
                    mesh->setPosition(position);
                }
            });
        }

        if (didFinishSetPosition) {
            didFinishSetPosition = false;
            position_min = position - 10.f;
            position_max = position + 10.f;
        }

        if (didSetRotation) {
            didSetRotation = false;
            AN::Dispatch::async(AN::Dispatch::Game, [rotation = rotation, _mesh = _mesh] {
                auto mesh = _mesh.lock();
                if (mesh) {
                    mesh->setRotation(rotation);
                }
            });
        }

        if (didSetScale) {
            didSetScale = false;
            AN::Dispatch::async(AN::Dispatch::Game, [scale = scale, _mesh = _mesh] {
                auto mesh = _mesh.lock();
                if (mesh) {
                    mesh->setScale(scale);
                }
            });
        }

        if (didFinishSetScale) {
            didFinishSetScale = false;
            scale_max = scale * 2.f;
        }
    }


};

class ImguiNode : public AN::ImguiNode {
    typedef ImguiNode Self;
    typedef AN::ImguiNode Super;


    /// render's data
    std::weak_ptr<AN::StaticMeshNode> mesh;

    virtual bool init() override { return false; }

public:

    struct ImguiNodeSceneProxy : Super::SceneProxyType {
        bool show_demo_window{ true };
        bool show_main_window{ true };

        int selected_fps{};

        int id;
        std::shared_ptr<TransformEdit> transformEdit;

        explicit ImguiNodeSceneProxy(AN::ImguiNode &node) : Super::SceneProxyType(node) {
            transformEdit = std::make_shared<TransformEdit>();
        }


        virtual void postRender(const AN::RenderContext &context) override {
            Super::SceneProxyType::postRender(context);
            newFrame(context);

            id = 0;

            if (show_demo_window) {
                ImGui::ShowDemoWindow(&show_demo_window);
            }

            if (show_main_window) {
                ImGui::Begin("Main Window", &show_main_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f * context.deltaTime, 1.f / context.deltaTime);

                static const char* canSelectFPS[] = { "60", "120", "140", "200", "300", "400", "INF"};
                if (ImGui::Combo("Set FPS", &selected_fps, canSelectFPS, std::size(canSelectFPS))) {
                    AN::Dispatch::async(AN::Dispatch::Game, [=, index = selected_fps]{
                        int fps;
                        switch (index) {
                            case 0:
                                fps = 60;
                                break;
                            case 1:
                                fps = 120;
                                break;
                            case 2:
                                fps = 140;
                                break;
                            case 3:
                                fps = 200;
                                break;
                            case 4:
                                fps = 300;
                                break;
                            case 5:
                                fps = 400;
                                break;
                            default:
                                fps = INT_MAX;
                        }
                        AN::GetGame().setMaxFrameRate(fps);
                    });
                }



                ImGui::Separator();

                ImGui::Text("Mouse Position X: %f Y: %f", AN::GetInputManager().getMousePositionX(), AN::GetInputManager().getMousePositionY());

                ImGui::Separator();

                transformEdit->draw();

                ImGui::End();
            }

            endFrame(context);

        }
    };

    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    ImguiNode() {
        tick = true;
    }

    virtual void updateSceneProxy() override {
        Super::updateSceneProxy();
        auto *proxy = (ImguiNodeSceneProxy *)sceneProxy;

        auto sMesh = mesh.lock();

        proxy->retain();
        AN::GetRenderQueue().enqueue([proxy, sMesh] {
            proxy->transformEdit->update();
            if (sMesh) {
                proxy->transformEdit->setMesh(sMesh);
            }
            proxy->release();
        });

    }


    virtual AN::RC::SceneProxy *createSceneProxy() override {
        return new ImguiNodeSceneProxy(*this);
    }

    virtual bool init(const std::shared_ptr<AN::StaticMeshNode> &aMesh) {
        Super::init();
        mesh = aMesh;
        return true;
    }

};


class MainNode : public AN::Node {
    typedef MainNode Self;
    typedef AN::Node Super;

    AN::InputBinding playerInputBinding;

    AN::InputBinding windowInputBinding;

    std::shared_ptr<AN::CameraNode> camera;
    std::shared_ptr<ImguiNode> imGuiNode;

    std::shared_ptr<AN::TextNode> label;

    AN::Math::vec3 moveMentInput;
public:
    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    MainNode() {
        tick = true;
    }

//    template<typename T>
//    void addSeparation(T &&node, const AN::Math::vec3 &position) {
//        auto copy = node->copy();
//        copy->setPosition(position);
//        addChild(copy);
//    }

    void addInputMappings() {
        AN::InputManager &manager = AN::GetInputManager();
        manager.addActionMapping("ShowEditWindow", AN::InputKey::P, AN::InputModifierFlags::Shift | AN::InputModifierFlags::Control);
        manager.addActionMapping("Quit", AN::InputKey::Esc);

        manager.addAxisMapping("CameraPitch", AN::InputKey::MouseY, 0.1f);
        manager.addAxisMapping("CameraYaw", AN::InputKey::MouseX, 0.1f);

        manager.addAxisMapping("MoveForward", AN::InputKey::W, 1.f);
        manager.addAxisMapping("MoveForward", AN::InputKey::S, -1.f);

        manager.addAxisMapping("MoveRight", AN::InputKey::A, -1.f);
        manager.addAxisMapping("MoveRight", AN::InputKey::D, 1.f);
    }

    void showEditWindow() {
        if (imGuiNode->isPostRender()) {
            imGuiNode->setPostRender(false);
            AN::Dispatch::async(AN::Dispatch::Main, [] {
                AN::Cursor::setState(AN::CursorState::Disabled);
            });
        } else {
            imGuiNode->setPostRender(true);
            AN::Dispatch::async(AN::Dispatch::Main, [] {
                AN::Cursor::setState(AN::CursorState::Normal);
            });
        }
    }


    void pitchCamera(float axisValue) {
        auto rotation = camera->getRotation();
        rotation.pitch = std::clamp(rotation.pitch - axisValue, -80.0f, 80.0f);
        camera->setRotation(rotation);
    }

    void yawCamera(float axisValue) {
        auto rotation = camera->getRotation();
        rotation.yaw -= axisValue;
        camera->setRotation(rotation);
    }


    void moveForward(float AxisValue) {
        moveMentInput.x = std::clamp(AxisValue, -1.0f, 1.0f);
    }

    void moveRight(float AxisValue) {
        moveMentInput.y = std::clamp(AxisValue, -1.0f, 1.0f);
    }


    void addInputBinding() {
       windowInputBinding.bindAction("ShowEditWindow", AN::InputEvent::Pressed, this, &MainNode::showEditWindow);

       windowInputBinding.bindAction("Quit", AN::InputEvent::Pressed, [] {
           AN::App->terminate();
       });

       playerInputBinding.bindAxis("CameraPitch", this, &MainNode::pitchCamera);
       playerInputBinding.bindAxis("CameraYaw", this, &MainNode::yawCamera);

       playerInputBinding.bindAxis("MoveForward", this, &MainNode::moveForward);
       playerInputBinding.bindAxis("MoveRight", this, &MainNode::moveRight);
    }

    virtual void update(float deltaTime) override {
        Super::update(deltaTime);

        label->setText(std::format("FPS: ({:.2f})", 1.f / deltaTime).c_str());

        moveMentInput = {};

        AN::GetInputManager().processInput(windowInputBinding);

        if (!imGuiNode->isPostRender()) {
            AN::GetInputManager().processInput(playerInputBinding);
        }


        AN::Math::normalize(moveMentInput);

        auto position = camera->getPosition();
        auto rotation = camera->getRotation();

        auto rotationMatrix = AN::Math::eulerAngleYXZ(AN::Math::radians(rotation.yaw), AN::Math::radians(rotation.pitch), 0.f);
        AN::Math::vec3 forwardVector = rotationMatrix * AN::Math::vec4(AN::CameraNode::GetDefaultForwardVector(), 1.f);
        forwardVector = AN::Math::normalize(forwardVector);
        AN::Math::vec3 rightVector = rotationMatrix * AN::Math::vec4(AN::CameraNode::GetDefaultRightVector(), 1.f);
        rightVector = AN::Math::normalize(rightVector);
        position += forwardVector * moveMentInput.x * 1000.f * deltaTime;
        position += rightVector * moveMentInput.y * 1000.f * deltaTime;

        camera->setPosition(position);

    }

    virtual bool init() override {
        Super::init();

        addInputMappings();
        addInputBinding();

        camera = AN::CameraNode::Alloc();
        if (!camera->init()) {
            return false;
        }

        camera->setPosition({ 0.f, 0.f, 3.f });

        addChild(camera);

        AN::StaticMeshNodeTextureInfo meshTexture{ .name = "Castle Benrath.jpg", .type = AN::TextureType::diffuse };

        auto mesh = AN::StaticMeshNode::Alloc();
        if (!mesh->init(cube_vertices, std::size(cube_vertices), cube_indices, std::size(cube_indices), &meshTexture, 1)) {
            return false;
        }

//        addSeparation(mesh, { 2.0f,  5.0f, -15.0f });
//        addSeparation(mesh, { -1.5f, -2.2f, -2.5f });
//        addSeparation(mesh, { -3.8f, -2.0f, -12.3f });
//        addSeparation(mesh, { 2.4f, -0.4f, -3.5f });
//        addSeparation(mesh, { -1.7f,  3.0f, -7.5f });
//        addSeparation(mesh, { 1.3f, -2.0f, -2.5f });
//        addSeparation(mesh, { 1.5f,  2.0f, -2.5f });
//        addSeparation(mesh, { 1.5f,  0.2f, -1.5f });
//        addSeparation(mesh, { -1.3f,  1.0f, -1.5f });



        AN::Geo::Sphere sphere(1.f, 12, 24);

        std::vector<AN::Vertex> sphereVertices;
        sphereVertices.reserve(sphere.vertices.size());

        for (int i = 0; i < sphere.vertices.size(); ++i) {
            AN::Vertex vertex;
            vertex.position = sphere.vertices[i];
            vertex.normal = sphere.normals[i];
            sphereVertices.push_back(vertex);
        }

        auto sphereMesh = AN::StaticMeshNode::Alloc();
        if (!sphereMesh->init(sphereVertices.data(), sphereVertices.size(), sphere.indices.data(), sphere.indices.size())) {
            return false;
        }

        sphereMesh->setPosition({ 0.f, 0.f, -10.f });

        addChild(sphereMesh);


        auto model = AN::StaticModelNode::Alloc();

        /// ./Resources/Models/Sponza/sponza.obj
        if (!model->init("./Resources/Models//Sponza/sponza.obj")) {
            return false;
        }

        model->setScale({ 10.f, 10.f, 10.f });

        addChild(model);

        label = AN::TextNode::Alloc();
//
        if (!label->init("FPS: (??.\?\?)", AN::Math::vec4(0.5, 0.8f, 0.2f, 1.f))) {
            return false;
        }

        label->setScale(0.5f);

        auto &fontAtlas = AN::GetFontManager().getDefaultFontAtlas();
        label->setPosition({ 10.0f, (float)(fontAtlas.getMaxFontHeight() - fontAtlas.getMaxUnderBaseline()) * 0.5f });


        auto welcomeText = AN::TextNode::Alloc();

        if (!welcomeText->init("Hello! \"ojoie \"!#$", AN::Math::vec4(0.3, 0.7f, 0.9f, 1.f))) {
            return false;
        }

        welcomeText->setScale(3.f);
        auto textSize = AN::GetFontManager().CalTextSize("Hello! \"ojoie \"!#$", 3.f, 3.f);
        welcomeText->setPosition({ 10.f, AN::GetGame().height / 2.f });


        imGuiNode = ImguiNode::Alloc();
        if (!imGuiNode->init(mesh)) {
            return false;
        }

        imGuiNode->setPostRender(false);

        addChild(mesh);
        addChild(imGuiNode);


        addChild(label);
        addChild(welcomeText);

        AN::Dispatch::async(AN::Dispatch::Main, [] {
            AN::Cursor::setState(AN::CursorState::Disabled);
        });


        return true;
    }

};


struct AppDelegate {

    std::unique_ptr<AN::Window> mainWindow;

    void bind(AN::Application *application) {
        application->didFinishLaunching.bind(this, &AppDelegate::applicationDidFinishLaunching);
        application->shouldTerminateAfterLastWindowClosed = AppDelegate::applicationShouldTerminateAfterLastWindowClosed;
        application->willTerminate.bind(this, &AppDelegate::applicationWillTerminate);
    }


    static bool applicationShouldTerminateAfterLastWindowClosed(AN::Application *application) {
        return true;
    }

    void applicationDidFinishLaunching(AN::Application *application) {
        mainWindow = std::make_unique<AN::Window>();
        AN::Size screenSize = AN::GetDefaultScreenSize();
        mainWindow->init({0, 0, screenSize.width / 2, screenSize.height / 2 });
        mainWindow->setTitle("3d-test");
        mainWindow->center();
        mainWindow->makeCurrentContext();
        mainWindow->makeKeyAndOrderFront();

        auto node = MainNode::Alloc();
        AN::GetGame().entryNode = node;
        AN::GetGame().setMaxFrameRate(AN::GetDefaultScreenRefreshRate() * 2);
    }

    void applicationWillTerminate(AN::Application *application) {
        mainWindow = nullptr;
    }


};

int main(int argc, const char * argv[]) {

    ANLog("application main start");

    AN::Application &app = AN::Application::GetSharedApplication();
    AppDelegate appDelegate;

    appDelegate.bind(&app);

    try {
        app.run();

    } catch (const std::exception &exception) {

        std::ofstream file("ojoie.log");
        file << exception.what();

        const auto &logs = AN::Log::GetSharedLog().getLogs();

        for (auto &log : logs) {
            file << log;
        }

        file.close(); /// file destructor may not be called if crashed

        printf("Exception: %s\n", exception.what());

#ifdef _WIN32
        MessageBoxA(nullptr, exception.what(), "Exception", MB_OK|MB_ICONERROR);
#endif

    }

    return 0;
}