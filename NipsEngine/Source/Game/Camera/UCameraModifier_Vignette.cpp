#include "UCameraModifier_Vignette.h"

void UCameraModifier_Vignette::ModifyCamera(float DeltaTime, FCameraEffectSettings& OutEffects)
{
    if (!bEnabled)
    {
        return;
    }

    OutEffects.VignetteIntensity = Intensity;
    OutEffects.VignetteRadius    = Radius;
    OutEffects.VignetteSoftness  = Softness;
}
