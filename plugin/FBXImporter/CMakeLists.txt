add_an_library(FBXImporter SHARED FBXImporter.cpp)
target_link_libraries(FBXImporter PRIVATE fbx ojoie)

if(VLD_MEM_CHECK AND VLD_FOUND)
    target_include_directories(FBXImporter PRIVATE ${VLD_INCLUDE_DIRS})
    target_link_libraries(FBXImporter PRIVATE ${VLD_LIBRARIES})
endif()