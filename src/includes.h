#pragma once

#if !defined(__EMSCRIPTEN__)
#define XR_SUPPORT
#define USE_MIRROR_WINDOW
#endif

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_RIGHT_HANDED
#define GLM_FORCE_QUAT_DATA_XYZW
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

enum eEYE {
    EYE_LEFT,
    EYE_RIGHT,
    EYE_COUNT // Let's assume this will never be different to 2...
};
