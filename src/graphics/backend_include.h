#pragma once

#if defined(BACKEND_DX12)
#include <dawn/native/D3D12Backend.h>
#elif defined(BACKEND_VULKAN)
#include "dawn/native/VulkanBackend.h"
#elif defined(BACKEND_METAL)
#include "dawn/native/MetalBackend.h"
#endif