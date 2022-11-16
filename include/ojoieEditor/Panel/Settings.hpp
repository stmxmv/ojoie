//
// Created by aojoie on 10/6/2022.
//

#ifndef OJOIE_SETTINGS_HPP
#define OJOIE_SETTINGS_HPP

#include <ojoieEditor/Panel/EditorPanel.hpp>

namespace AN::Editor {

class Settings : public Panel {
    typedef Settings Self;
    typedef Panel Super;
    bool show;
    int selected_fps{};
public:
    static std::shared_ptr<Self> Alloc() {
        return std::make_shared<Self>();
    }

    virtual bool init() override {
        if (Super::init()) {
            show = true;
            return true;
        }
        return false;
    }

    virtual void draw(const RenderContext &context) override {
        Super::draw(context);
        if (show) {
            ImGui::Begin("Main Window", &show);
            // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

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

            ImGui::End();
        }

    }
};

}

#endif//OJOIE_SETTINGS_HPP
