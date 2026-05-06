#pragma once
#include "Engine/Render/Scene/RenderBus.h"

// Unreal CameraModifier 패턴: 매 프레임 FCameraEffectSettings를 수정한다.
class UCameraModifier
{
public:
    virtual ~UCameraModifier() = default;

    virtual void ModifyCamera(float DeltaTime, FCameraEffectSettings& OutEffects) = 0;

    // true를 반환하면 UPlayerCameraManager가 다음 Tick에 자동으로 제거한다.
    // 기본값 false = 명시적으로 RemoveModifier()를 호출할 때까지 영구 유지 (Vignette, LetterBox 등)
    virtual bool IsFinished() const { return false; }

    int32 Priority = 0;
    bool bEnabled = true;
};
