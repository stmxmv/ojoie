
add_library(dxc SHARED IMPORTED GLOBAL)

set_target_properties(
        dxc PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_DIR}/dxc"
        IMPORTED_IMPLIB "${THIRD_PARTY_DIR}/dxc/lib/x64/dxcompiler.lib"
        IMPORTED_LOCATION "${THIRD_PARTY_DIR}/dxc/bin/x64/dxcompiler.dll"
)