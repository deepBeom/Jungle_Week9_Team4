#pragma once

#include "Core/CoreTypes.h"

// Water rendering uses a dedicated shader override, but still flows through the
// normal forward static-mesh path. Keep the resource contract centralized here
// so C++ binding code and HLSL register comments stay in sync.
namespace WaterShaderBindings
{
    constexpr uint32 MaterialConstantBuffer = 2u;   // b2 in Shaders/Water.hlsl
    constexpr uint32 FirstTextureRegister = 0u;     // t0 = WaterNormalA
    constexpr uint32 NormalATexture = 0u;           // t0
    constexpr uint32 NormalBTexture = 1u;           // t1
    constexpr uint32 DiffuseTexture = 2u;           // t2
    constexpr uint32 TextureCount = 3u;             // t0-t2
    constexpr uint32 GlobalLightInfoConstantBuffer = 3u; // b3
    constexpr uint32 GlobalLightStructuredBuffer = 3u;   // t3
    constexpr uint32 VisibleLightInfoConstantBuffer = 4u; // b4
    constexpr uint32 PointLightStructuredBuffer = 8u;     // t8
}

namespace WaterMaterialParameterNames
{
    constexpr const char* IsWater = "bIsWater";
    constexpr const char* DiffuseMap = "DiffuseMap";
    constexpr const char* NormalMap = "NormalMap";
    constexpr const char* WaterNormalA = "WaterNormalA";
    constexpr const char* WaterNormalB = "WaterNormalB";
}

namespace WaterRenderingLimits
{
    constexpr uint32 MaxLocalLights = 8u;
}

namespace WaterDefaultAssets
{
    // Default editor spawn asset only. Runtime water logic must not assume a
    // specific mesh beyond "reasonable flat water surface".
    constexpr const char* MeshPath = "Asset/Mesh/Water/Wave.obj";
    constexpr const char* MeshDirectoryA = "Asset/Mesh/Water/";
    constexpr const char* MeshDirectoryB = "Asset\\Mesh\\Water\\";
}
