aux_source_directory(lib YAML_SRCS)
add_an_library(yaml OBJECT ${YAML_SRCS})
target_include_directories(yaml PUBLIC include)
target_compile_definitions(yaml PRIVATE YAML_DECLARE_EXPORT INTERFACE YAML_DECLARE_STATIC)