add_library(volk OBJECT volk.c)

find_package(Vulkan REQUIRED FATAL_ERROR)

target_include_directories(
        volk
        PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/include/volk
        PUBLIC ${Vulkan_INCLUDE_DIRS}
)