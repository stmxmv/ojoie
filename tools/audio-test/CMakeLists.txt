add_an_tool(audio-test WIN32 main.cpp .manifest)

target_link_libraries(audio-test PRIVATE ojoie fmod)

if(VLD_MEM_CHECK AND VLD_FOUND)
    target_include_directories(audio-test PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(audio-test PRIVATE ${VLD_LIBRARIES})
endif()