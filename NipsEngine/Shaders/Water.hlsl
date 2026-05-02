#ifndef NIPS_SAMPLE_STATE_DECLARED
#define NIPS_SAMPLE_STATE_DECLARED
SamplerState SampleState : register(s0);
#endif

// b0: shared frame data populated from FShaderBindingInstance::ApplyFrameParameters
cbuffer WaterFrame : register(b0)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    float3 CameraPosition;
    float _WaterFramePad0;
    float bIsWireframe;
    float bLightingEnabled;
    float _WaterFramePad1;
    float _WaterFramePad2;
    float3 WireframeRGB;
    float _WaterFramePad3;
}

// b1: per-object transform data populated from FPerObjectConstants
cbuffer WaterPerObject : register(b1)
{
    row_major float4x4 Model;
    row_major float4x4 WorldInvTrans;
    float4 PrimitiveColor;
}

// b2: Stage 1 water controls
// Time drives UV scrolling. Later stages can extend this block with specular/depth-foam data.
cbuffer WaterMaterial : register(b2)
{
    float Time;
    float NormalStrength;
    float Alpha;
    float _WaterMaterialPad0;

    float2 NormalTilingA;
    float2 NormalScrollSpeedA;

    float2 NormalTilingB;
    float2 NormalScrollSpeedB;

    float3 BaseColor;
    float _WaterMaterialPad1;

    uint bHasNormalMapA;
    uint bHasNormalMapB;
    float ColorVariationStrength;
    float _WaterMaterialPad2;
}

// t0/t1: optional water normal or noise textures.
Texture2D WaterNormalA : register(t0);
Texture2D WaterNormalB : register(t1);

struct FWaterVSInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
};

struct FWaterPSInput
{
    float4 ClipPos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 WorldTangent : TEXCOORD3;
    float3 WorldBitangent : TEXCOORD4;
};

struct FWaterPSOutput
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 WorldPos : SV_TARGET2;
};

float4 ApplyWaterMVP(float3 Position)
{
    const float4 WorldPosition = mul(float4(Position, 1.0f), Model);
    const float4 ViewPosition = mul(WorldPosition, View);
    return mul(ViewPosition, Projection);
}

float3 BuildFallbackWorldTangent(float3 WorldNormal)
{
    const float3 UpCandidate = abs(WorldNormal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
    return normalize(cross(UpCandidate, WorldNormal));
}

float3 ResolveWaterWorldNormal(FWaterPSInput Input, float2 UV)
{
    float3 GeometricNormal = normalize(Input.WorldNormal);

    float3 N1 = float3(0.0f, 0.0f, 1.0f);
    if (bHasNormalMapA != 0u)
    {
        const float2 UV1 = UV * NormalTilingA + Time * NormalScrollSpeedA;
        N1 = WaterNormalA.Sample(SampleState, UV1).xyz * 2.0f - 1.0f;
    }

    float3 N2 = float3(0.0f, 0.0f, 1.0f);
    if (bHasNormalMapB != 0u)
    {
        const float2 UV2 = UV * NormalTilingB + Time * NormalScrollSpeedB;
        N2 = WaterNormalB.Sample(SampleState, UV2).xyz * 2.0f - 1.0f;
    }

    float3 WaterNormalTS = normalize(N1 + N2);
    WaterNormalTS.xy *= NormalStrength;
    WaterNormalTS = normalize(WaterNormalTS);

    float3 WorldTangent = Input.WorldTangent - GeometricNormal * dot(Input.WorldTangent, GeometricNormal);
    if (dot(WorldTangent, WorldTangent) <= 1.0e-8f)
    {
        WorldTangent = BuildFallbackWorldTangent(GeometricNormal);
    }
    else
    {
        WorldTangent = normalize(WorldTangent);
    }

    float3 WorldBitangent = Input.WorldBitangent;
    WorldBitangent = WorldBitangent - GeometricNormal * dot(WorldBitangent, GeometricNormal);
    WorldBitangent = WorldBitangent - WorldTangent * dot(WorldBitangent, WorldTangent);
    if (dot(WorldBitangent, WorldBitangent) <= 1.0e-8f)
    {
        WorldBitangent = normalize(cross(GeometricNormal, WorldTangent));
    }
    else
    {
        WorldBitangent = normalize(WorldBitangent);
    }

    const float3x3 TBN = float3x3(WorldTangent, WorldBitangent, GeometricNormal);
    return normalize(mul(WaterNormalTS, TBN));
}

FWaterPSInput mainVS(FWaterVSInput Input)
{
    FWaterPSInput Output;
    Output.WorldPos = mul(float4(Input.Position, 1.0f), Model).xyz;
    Output.WorldNormal = normalize(mul(Input.Normal, (float3x3)WorldInvTrans));
    Output.WorldTangent = normalize(mul(Input.Tangent, (float3x3)Model));
    Output.WorldBitangent = normalize(mul(Input.Bitangent, (float3x3)Model));
    Output.UV = Input.UV;
    Output.ClipPos = ApplyWaterMVP(Input.Position);
    return Output;
}

FWaterPSOutput mainPS(FWaterPSInput Input)
{
    FWaterPSOutput Output;

    const float3 WaterWorldNormal = ResolveWaterWorldNormal(Input, Input.UV);
    const float3 ViewDir = normalize(CameraPosition - Input.WorldPos);
    const float Fresnel = pow(1.0f - saturate(dot(WaterWorldNormal, ViewDir)), 3.0f);
    const float WaveTint = WaterWorldNormal.x * 0.5f + WaterWorldNormal.y * 0.5f;

    // Stage 1: subtle animated color response only. Specular/refraction/foam are reserved for later stages.
    float3 FinalColor = BaseColor;
    FinalColor *= (1.0f + WaveTint * ColorVariationStrength);
    FinalColor += Fresnel * (0.08f * PrimitiveColor.rgb);
    FinalColor *= PrimitiveColor.rgb;

    if (bIsWireframe > 0.5f)
    {
        FinalColor = WireframeRGB;
    }

    Output.Color = float4(saturate(FinalColor), saturate(Alpha * PrimitiveColor.a));
    Output.Normal = float4(WaterWorldNormal * 0.5f + 0.5f, 1.0f);
    Output.WorldPos = float4(Input.WorldPos, 1.0f);
    return Output;
}
