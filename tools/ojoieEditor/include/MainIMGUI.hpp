//
// Created by aojoie on 6/24/2023.
//

#pragma once
#include "EditorPanel.hpp"

#include <ojoie/IMGUI/IMGUI.hpp>

namespace AN::Editor {

class MainIMGUI : public IMGUI {

    std::vector<RefCountedPtr<Panel>> _editorPanels;

    /// main state variable
    bool play;
    bool pause;

    AN_CLASS(MainIMGUI, IMGUI)

public:

    explicit MainIMGUI(ObjectCreationMode mode);

    virtual bool init() override;
    virtual void dealloc() override;

    virtual void onGUI() override;

    template<typename T>
    void addPanel() {
        RefCountedPtr<Panel> panel = ref_transfer(new T());
        _editorPanels.push_back(panel);
    }

    template<typename T>
    T *findPanelOfType() {
        for (auto &panelPtr : _editorPanels) {
            auto &panel = *panelPtr;
            if (typeid(panel) == typeid(T)) {
                return (T *)panelPtr.get();
            }
        }
        return nullptr;
    }
};

MainIMGUI &GetMainIMGUI();


}
