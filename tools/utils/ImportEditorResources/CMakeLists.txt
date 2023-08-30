add_an_tool(ImportEditorResources main.cpp)
target_link_libraries(ImportEditorResources PRIVATE ojoie)

if (VLD_MEM_CHECK AND VLD_FOUND)
    target_include_directories(ImportEditorResources PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(ImportEditorResources PRIVATE ${VLD_LIBRARIES})
endif ()

target_compile_definitions(ImportEditorResources PRIVATE EDITOR_RESOURCE_ROOT="${CMAKE_SOURCE_DIR}/tools/ojoieEditor/Assets")