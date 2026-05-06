#pragma once
#include "UCameraModifier.h"

class UCameraModifier_Vignette : public UCameraModifier
{
public:
    void ModifyCamera(float DeltaTime, FCameraEffectSettings& OutEffects) override;

    float Intensity = 0.6f;   // 비네트 강도
    float Radius = 0.75f;     // 중앙 클리어 반경 (NDC 기준, ~0.5–1.2)
    float Softness = 0.45f;   // 페이드 소프트니스
};
