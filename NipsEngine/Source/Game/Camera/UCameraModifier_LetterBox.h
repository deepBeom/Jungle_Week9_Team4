#pragma once
#include "UCameraModifier.h"

class UCameraModifier_LetterBox : public UCameraModifier
{
public:
    void ModifyCamera(float DeltaTime, FCameraEffectSettings& OutEffects) override;

    // 목표 종횡비 (예: 2.35 = 시네마스코프, 1.85 = 아카데미)
    float AspectRatio = 2.35f;
};
