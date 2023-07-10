
#include "ojoie/Configuration/typedef.h"

#ifdef OJOIE_USE_VULKAN

#include "vulkan/ImguiNode.cpp"

#elif defined(OJOIE_USE_OPENGL)

#include "opengl/ImguiNode.cpp"

#endif