// Camera cinematic effects: Vignette → LetterBox → Fade
// Reads the previous pass result and outputs to SceneColor.

Texture2D SceneColor : register(t0);
SamplerState SampleState : register(s0);

cbuffer CameraEffectConstants : register(b10)
{
    float3 FadeColor;
    float  FadeAlpha;          // 0=no overlay, 1=fully covered

    float  LetterBoxRatio;     // target aspect ratio (0=disabled, e.g. 2.35)
    float  CurrentAspectRatio; // viewport width / height
    float  VignetteIntensity;  // 0=disabled
    float  VignetteRadius;     // NDC distance where vignette starts (~0.5–1.0)

    float  VignetteSoftness;
    float3 EffectPadding;
}

struct VSOutput
{
    float4 ClipPos : SV_POSITION;
    float2 UV      : TEXCOORD0;
};

// Fullscreen triangle: covers [-1,1] clip space with 3 vertices
VSOutput mainVS(uint VertexID : SV_VertexID)
{
    VSOutput Out;
    float2 Pos;
    if (VertexID == 0) Pos = float2(-1.0f, -1.0f);
    else if (VertexID == 1) Pos = float2(-1.0f,  3.0f);
    else                    Pos = float2( 3.0f, -1.0f);

    Out.ClipPos = float4(Pos, 0.0f, 1.0f);
    // DX11 clip-space Y is flipped relative to UV
    Out.UV = float2(Pos.x * 0.5f + 0.5f, -Pos.y * 0.5f + 0.5f);
    return Out;
}

float4 mainPS(VSOutput In) : SV_TARGET
{
    float2 UV = In.UV;
    float3 Color = SceneColor.Sample(SampleState, UV).rgb;

    // --- Vignette ---
    if (VignetteIntensity > 0.0f)
    {
        // NDC coords centered at origin, corrected for aspect ratio
        float2 NDC  = UV * 2.0f - 1.0f;
        NDC.x      *= CurrentAspectRatio;
        float Dist  = length(NDC);
        float Mask  = 1.0f - smoothstep(VignetteRadius - VignetteSoftness, VignetteRadius, Dist);
        // lerp toward black based on intensity
        Color = lerp(Color, Color * Mask, VignetteIntensity);
    }

    // --- LetterBox ---
    if (LetterBoxRatio > 0.0f && CurrentAspectRatio > 0.0f)
    {
        float Ratio = CurrentAspectRatio / LetterBoxRatio;
        if (Ratio < 1.0f)
        {
            // Viewport is taller than target → horizontal bars
            float BarH = (1.0f - Ratio) * 0.5f;
            if (UV.y < BarH || UV.y > 1.0f - BarH)
                Color = float3(0.0f, 0.0f, 0.0f);
        }
        else
        {
            // Viewport is wider than target → vertical bars (pillarbox)
            float BarW = (1.0f - 1.0f / Ratio) * 0.5f;
            if (UV.x < BarW || UV.x > 1.0f - BarW)
                Color = float3(0.0f, 0.0f, 0.0f);
        }
    }

    // --- Fade overlay ---
    if (FadeAlpha > 0.0f)
        Color = lerp(Color, FadeColor, FadeAlpha);

    return float4(Color, 1.0f);
}
