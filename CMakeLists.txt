cmake_minimum_required(VERSION 3.22)
project(ojoie VERSION 1.0)

# /////////// compiler specific settings
if (MSVC)
    set(CMAKE_CXX_STANDARD 23)
    add_compile_options($<$<CONFIG:Debug>:/analyze> /Zc:__cplusplus /await:strict /source-charset:utf-8 /execution-charset:utf-8)
    add_compile_definitions(NOMINMAX)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
else()
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-nullability-completeness")
endif()

message("debug flag: ${CMAKE_CXX_FLAGS_DEBUG}")
message("release with debug info flag: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message("release flag: ${CMAKE_CXX_FLAGS_RELEASE}")
# /////////// compiler specific settings

# /////////// platform specific settings
if(APPLE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    # append third-party lib
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        #    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/thirdparty/osx)
    endif()
elseif(WIN32)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/thirdparty/win32)
endif()
# /////////// platform specific settings

# /////////// project cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
include(AN)

include_directories(thirdparty/include)

# /////////// project cmake modules

add_subdirectory(thirdparty)
add_subdirectory(lib)
add_subdirectory(tools)


option(AN_BUILD_TEST "build tests" OFF)

if (AN_BUILD_TEST)
    message("Build tests is on")
    include(FetchContent)

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        release-1.12.1
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    #     not install googletest
    set(INSTALL_GMOCK OFF CACHE BOOL "not install test" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "not install test" FORCE)

    add_subdirectory(test)

endif()



# finally include install module
include(install)
