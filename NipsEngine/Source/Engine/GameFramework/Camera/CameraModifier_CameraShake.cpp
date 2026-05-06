#include "GameFramework/Camera/CameraModifier_CameraShake.h"

#include "Math/Utils.h"

#include <cmath>

namespace
{
    // Smooth envelope helper: soft in/out for reducing abrupt shake cut-off.
    float SmoothStep01(float Alpha)
    {
        const float ClampedAlpha = MathUtil::Clamp(Alpha, 0.0f, 1.0f);
        return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
    }
}

FCameraShakeModifier::FCameraShakeModifier(float InAmplitude, float InFrequency, float InDuration)
    : FTimedCameraModifier(MathUtil::Clamp(InDuration, 0.0f, 10.0f))
    , Amplitude(MathUtil::Clamp(InAmplitude, 0.0f, 100.0f))
    , Frequency(MathUtil::Clamp(InFrequency, 0.0f, 1000.0f))
{
    Priority = 16;
}

bool FCameraShakeModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    // The base class handles duration and normalized progress.
    float NormalizedTime = 1.0f;
    if (!Advance(DeltaTime, NormalizedTime))
    {
        return false;
    }

    // Fade out toward the end to avoid a visible camera "snap" when stopping.
    const float Envelope = 1.0f - SmoothStep01(NormalizedTime);
    const float AngularTime = GetElapsedSeconds() * Frequency * MathUtil::TwoPi;

    // Use multiple phase/frequency channels so shake feels less repetitive.
    const float RightOffset = std::sin(AngularTime) * Amplitude * Envelope;
    const float UpOffset = std::cos(AngularTime * 1.37f + 1.1f) * Amplitude * 0.6f * Envelope;
    const float ForwardOffset = std::sin(AngularTime * 0.73f + 2.2f) * Amplitude * 0.2f * Envelope;

    // Apply in camera-local axes to keep shake direction relative to look direction.
    const FVector Forward = InOutPOV.Rotation.GetForwardVector();
    const FVector Right = InOutPOV.Rotation.GetRightVector();
    const FVector Up = InOutPOV.Rotation.GetUpVector();

    InOutPOV.Location += Right * RightOffset;
    InOutPOV.Location += Up * UpOffset;
    InOutPOV.Location += Forward * ForwardOffset;

    return true;
}
