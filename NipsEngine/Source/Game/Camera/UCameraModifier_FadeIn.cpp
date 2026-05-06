#include "UCameraModifier_FadeIn.h"
#include <algorithm>

void UCameraModifier_FadeIn::ModifyCamera(float DeltaTime, FCameraEffectSettings& OutEffects)
{
    if (!bEnabled || !bIsActive)
    {
        return;
    }

    ElapsedTime += DeltaTime;
    const float T = (Duration > 0.0f) ? std::min(ElapsedTime / Duration, 1.0f) : 1.0f;

    // bFadeIn: 1→0 (검정 덮개가 걷힘), !bFadeIn: 0→1 (검정으로 덮임)
    CurrentAlpha = bFadeIn ? (1.0f - T) : T;

    if (CurrentAlpha > 0.0f)
    {
        OutEffects.FadeAlpha = std::max(OutEffects.FadeAlpha, CurrentAlpha);
        OutEffects.FadeColor = FadeColor;
    }

    if (T >= 1.0f)
    {
        bIsActive = false;
    }
}

void UCameraModifier_FadeIn::StartFade(float InDuration, bool bInFadeIn, FVector InColor)
{
    Duration = InDuration;
    bFadeIn = bInFadeIn;
    FadeColor = InColor;
    ElapsedTime = 0.0f;
    CurrentAlpha = bFadeIn ? 1.0f : 0.0f;
    bIsActive = true;
}

void UCameraModifier_FadeIn::StopFade()
{
    bIsActive = false;
    CurrentAlpha = 0.0f;
}

