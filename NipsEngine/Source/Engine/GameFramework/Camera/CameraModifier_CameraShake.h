#pragma once

#include "GameFramework/Camera/CameraModifier.h"

// Procedural camera shake using sinusoidal offsets in local camera axes.
// The shake only affects the transient final POV for the current frame.
class FCameraShakeModifier final : public FTimedCameraModifier
{
public:
    // Amplitude: positional intensity in world units.
    // Frequency: oscillation speed in Hz-like scalar.
    // Duration: active lifetime in seconds.
    FCameraShakeModifier(float InAmplitude, float InFrequency, float InDuration);

    bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
    // Maximum displacement scale.
    float Amplitude = 0.0f;
    // Oscillation frequency multiplier.
    float Frequency = 0.0f;
};
