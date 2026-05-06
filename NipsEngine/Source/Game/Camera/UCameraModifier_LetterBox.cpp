#include "UCameraModifier_LetterBox.h"

void UCameraModifier_LetterBox::ModifyCamera(float DeltaTime, FCameraEffectSettings& OutEffects)
{
    if (!bEnabled)
    {
        return;
    }

    OutEffects.LetterBoxRatio = AspectRatio;
}
