

add_library(FreeType SHARED IMPORTED GLOBAL)

if (WIN32)
    set(FreeType_DIR "${CMAKE_SOURCE_DIR}/thirdparty/win32/FreeType")

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(FreeType_LIB "${FreeType_DIR}/lib/win64/freetype")
    else()
        set(FreeType_LIB "${FreeType_DIR}/lib/win32/freetype")
    endif()

    set_target_properties(
            FreeType PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FreeType_DIR}/include"
            IMPORTED_IMPLIB "${FreeType_LIB}.lib"
            IMPORTED_LOCATION "${FreeType_LIB}.dll"
    )
else()

endif ()


