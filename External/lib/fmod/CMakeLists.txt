
add_library(fmod SHARED IMPORTED GLOBAL)

set(FMOD_DIR "C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows")

set_target_properties(
        fmod PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR}/api/core/inc"
        IMPORTED_IMPLIB "${FMOD_DIR}/api/core/lib/x64/fmod_vc.lib"
        IMPORTED_LOCATION "${FMOD_DIR}/api/core/lib/x64/fmod.dll"
)