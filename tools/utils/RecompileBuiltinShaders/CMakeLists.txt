add_an_tool(RecompileBuiltinShaders main.cpp)
target_link_libraries(RecompileBuiltinShaders PRIVATE ojoie)

if (VLD_MEM_CHECK AND VLD_FOUND)
    target_include_directories(RecompileBuiltinShaders PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(RecompileBuiltinShaders PRIVATE ${VLD_LIBRARIES})
endif ()

target_compile_definitions(RecompileBuiltinShaders PRIVATE AN_SHADER_ROOT="${CMAKE_SOURCE_DIR}/lib/ojoie/Shaders")