

#include "Core/typedef.h"

#ifdef OJOIE_USE_VULKAN
#include "vulkan/RenderPipeline.cpp"

#elif defined(OJOIE_USE_OPENGL)
#include "opengl/RenderPipeline.cpp"
#endif