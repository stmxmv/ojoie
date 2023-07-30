add_an_tool(
        ojoieEditor WIN32
        main.cpp
        resources/resource.rc
        resources/.manifest

        src/AppDelegate.cpp
        src/MainIMGUI.cpp
        src/Picking.cpp

        src/Panels/ViewportPanel.cpp
        src/Panels/ConsolePanel.cpp
        src/Panels/InspectorPanel.cpp
        src/Panels/HierarchyPanel.cpp
        src/Panels/ProjectPanel.cpp

        src/Project/Project.cpp
)

target_link_libraries(ojoieEditor PRIVATE ojoieEditorRuntime)

target_include_directories(ojoieEditor PRIVATE include)

if (VLD_MEM_CHECK AND VLD_FOUND)
    target_include_directories(ojoieEditor PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(ojoieEditor PRIVATE ${VLD_LIBRARIES})
endif ()
