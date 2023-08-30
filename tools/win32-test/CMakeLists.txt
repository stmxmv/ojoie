add_an_tool(win32-test WIN32 .manifest main.cpp)

target_link_libraries(win32-test PRIVATE ojoie)

if (VLD_MEM_CHECK AND VLD_FOUND)
        target_include_directories(win32-test PRIVATE ${VLD_INCLUDE_DIRS})
        target_link_libraries(win32-test PRIVATE ${VLD_LIBRARIES})
endif ()

if (USE_SANITIZER)
    target_link_directories(win32-test PRIVATE "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/lib/clang/15.0.1/lib/windows")
    target_link_libraries(win32-test PRIVATE clang_rt.asan_dynamic-x86_64 clang_rt.asan_dynamic_runtime_thunk-x86_64)
    target_link_options(win32-test PRIVATE /wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib)
    target_link_libraries(win32-test PRIVATE imgui)
endif()