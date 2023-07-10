add_library(detours INTERFACE ../../tools/ojoieEditor/include/Selection.hpp ../../tools/ojoieEditor/src/Selection.cpp)

target_include_directories(detours INTERFACE include)

target_link_libraries(detours INTERFACE ${CMAKE_CURRENT_LIST_DIR}/lib/detours.lib)