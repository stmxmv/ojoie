add_an_library(CrashHandler SHARED CrashHandler.cpp)

target_link_libraries(CrashHandler PRIVATE ojoie detours)

target_include_directories(CrashHandler PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if(VLD_MEM_CHECK AND VLD_FOUND)
    target_include_directories(CrashHandler PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(CrashHandler PRIVATE ${VLD_LIBRARIES})
endif()