add_library(detours INTERFACE ../../include/ojoie/Geometry/Plane.hpp)

target_include_directories(detours INTERFACE include)

target_link_libraries(detours INTERFACE ${CMAKE_CURRENT_LIST_DIR}/lib/detours.lib)