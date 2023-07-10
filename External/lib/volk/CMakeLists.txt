add_library(volk OBJECT volk.c)

find_package(Vulkan REQUIRED)

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK is not found!")
endif()

target_include_directories(
        volk
        PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/include/volk
        PUBLIC ${Vulkan_INCLUDE_DIRS}
)