#pragma once
#include "UCameraModifier.h"

// 씬을 단색으로 페이드 인/아웃시키는 카메라 모디파이어.
// StartFade()로 시작, IsFadeComplete()로 완료 여부 확인.
class UCameraModifier_FadeIn : public UCameraModifier
{
public:
    void ModifyCamera(float DeltaTime, FCameraEffectSettings& OutEffects) override;

    // bFadeIn=true: 검정→씬 (페이드 인), false: 씬→검정 (페이드 아웃)
    void StartFade(float InDuration, bool bInFadeIn, FVector InColor = { 0.0f, 0.0f, 0.0f });
    void StopFade();
    bool IsFinished() const override { return !bIsActive; }
    bool IsActive() const { return bIsActive; }

    float GetCurrentAlpha() const { return CurrentAlpha; }

private:
    float Duration = 1.0f;
    float ElapsedTime = 0.0f;
    bool bFadeIn = true;
    bool bIsActive = false;
    float CurrentAlpha = 0.0f;
    FVector FadeColor = { 0.0f, 0.0f, 0.0f };
};
