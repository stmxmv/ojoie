cmake_minimum_required(VERSION 3.22)
project(ojoie VERSION 1.0)

# /////////// compiler specific settings
option(USE_SANITIZER "use AddressSanitizer" FALSE)

set(BUILD_SHARED_LIBS TRUE)
if (MSVC)
    set(CMAKE_CXX_STANDARD 23)
    set(AN_COMPILE_OPTIONS #[[$<$<CONFIG:Debug>:/analyze>]] /await /Zc:__cplusplus /utf-8 /EHsc)
    if (USE_SANITIZER)
        set(BUILD_SHARED_LIBS FALSE)
        set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
        set(AN_COMPILE_OPTIONS ${AN_COMPILE_OPTIONS} -fsanitize=address /EHsc)
        add_compile_definitions(_DISABLE_STRING_ANNOTATION=1 _DISABLE_VECTOR_ANNOTATION=1)
    endif()
    # generate pdb file for release
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    add_compile_definitions(NOMINMAX _UNICODE UNICODE)
#    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
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
option(OJOIE_USE_VULKAN "use vulkan api" FALSE)
if(APPLE)
    set(OJOIE_USE_VULKAN FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    # append external lib
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        #    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/External/osx)
    endif()
elseif(WIN32)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/External/win32)
    set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/External/win32)
endif()

# /////////// platform specific settings

# /////////// project cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
include(AN)
include(GenerateExportHeader)

include_directories(External/include)

# /////////// project cmake modules


option(VLD_MEM_CHECK "use vld to check memory" OFF)
if(VLD_MEM_CHECK)
    find_package(VLD)
    if(${VLD_FOUND})
        message("found vld at ${VLD_ROOT_DIR}")
    endif()
    add_compile_definitions($<$<CONFIG:Debug>:VLD_MEM_CHECK>)
endif()

# finding required package

macro(an_find_package name)
    find_package(${name} ${ARGN})
    if (${name}_FOUND)
        message("found ${name} at ${${name}_DIR}")
    else()
        message(FATAL_ERROR "${name} not found!")
    endif()
endmacro()

#an_find_package(Fbx)

if (WIN32)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/External/DirectTex/win32/shared)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/External/TBB)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/External/FBX/2020.2.1)
endif()

add_subdirectory(External)
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(plugin)


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
