#include "../Common.hlsl"

// scene composition과 swap chain output을 분리하는 최종 출력 pass.
Texture2D SceneFinalColor : register(t0);
SamplerState SampleState : register(s0);

cbuffer PresentSettings : register(b0)
{
    float FadeAmount;
    float3 FadeColor;
    float LetterBoxAmount;
    uint bGammaCorrectionEnabled;
    float Gamma;
    uint bVignetteEnabled;
    float VignetteIntensity;
    float VignetteRadius;
};

struct VSOutput
{
    float4 ClipPos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

VSOutput mainVS(uint VertexID : SV_VertexID)
{
    VSOutput Output;

    if (VertexID == 0)
    {
        Output.ClipPos = float4(-1.0f, -1.0f, 0.0f, 1.0f);
        Output.UV = float2(0.0f, 1.0f);
    }
    else if (VertexID == 1)
    {
        Output.ClipPos = float4(-1.0f, 3.0f, 0.0f, 1.0f);
        Output.UV = float2(0.0f, -1.0f);
    }
    else
    {
        Output.ClipPos = float4(3.0f, -1.0f, 0.0f, 1.0f);
        Output.UV = float2(2.0f, 1.0f);
    }

    return Output;
}

float4 mainPS(VSOutput Input) : SV_TARGET
{
    // 최종 scene result를 그대로 출력한다.
    float2 UV = saturate(Input.UV);
    float3 Color = SceneFinalColor.Sample(SampleState, UV).rgb;

    if (bVignetteEnabled != 0 && VignetteIntensity > 0.0f)
    {
        float2 CenteredUV = (UV - 0.5f) / 0.70710678f;
        float DistanceFromCenter = length(CenteredUV);
        float EdgeAlpha = smoothstep(VignetteRadius, 1.0f, DistanceFromCenter);
        Color *= 1.0f - saturate(EdgeAlpha * VignetteIntensity);
    }

    if (FadeAmount > 0.0f)
    {
        Color = lerp(Color, FadeColor, saturate(FadeAmount));
    }

    if (bGammaCorrectionEnabled != 0 && Gamma > 0.0f)
    {
        Color = pow(saturate(Color), 1.0f / Gamma);
    }

    if (LetterBoxAmount > 0.0f)
    {
        bool bInLetterBox = UV.y <= LetterBoxAmount || UV.y >= (1.0f - LetterBoxAmount);
        if (bInLetterBox)
        {
            Color = 0.0f.xxx;
        }
    }

    return float4(Color, 1.0f);
}
