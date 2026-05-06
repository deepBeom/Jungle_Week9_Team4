#pragma once

#include "GameFramework/Camera/CameraModifier.h"

enum class ECameraShakePatternType
{
    WaveOscillator,
    CameraSequence,
};

struct FCameraShakeWaveOscillatorParams
{
    FVector LocationAmplitude = FVector::ZeroVector;
    FVector LocationFrequency = FVector::ZeroVector;
    FVector RotationAmplitude = FVector::ZeroVector;
    FVector RotationFrequency = FVector::ZeroVector;
    float FOVAmplitude = 0.0f;
    float FOVFrequency = 0.0f;
};

struct FCameraShakeParams
{
    ECameraShakePatternType PatternType = ECameraShakePatternType::WaveOscillator;
    float Duration = 0.0f;
    FCameraShakeWaveOscillatorParams WaveOscillator;
};

struct FCameraShakePatternUpdateResult
{
    FCameraShakePatternUpdateResult()
        : Location(FVector::ZeroVector), Rotation(FQuat::Identity), FOV(0.0f)
    {
    }

    FVector Location = FVector::ZeroVector;
    FQuat Rotation = FQuat::Identity;
    float FOV = 0.0f;
};

class ICameraShakePattern
{
public:
    virtual ~ICameraShakePattern() = default;
    virtual FCameraShakePatternUpdateResult UpdatePattern(float DeltaTime, float ElapsedTime, float TotalDuration) = 0;
};

class FCameraShakeModifier final : public FTimedCameraModifier
{
public:
    // TODO: Blend in/out time 추가 필요
    explicit FCameraShakeModifier(const FCameraShakeParams& Params);

    bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
    std::unique_ptr<ICameraShakePattern> ShakePattern;
};

class FWaveOscillatorPattern : public ICameraShakePattern
{
public:
    explicit FWaveOscillatorPattern(const FCameraShakeWaveOscillatorParams& InParams)
        : Params(InParams)
    {
    }

    FCameraShakePatternUpdateResult UpdatePattern(float DeltaTime, float ElapsedTime, float TotalDuration) override;

private:
    FCameraShakeWaveOscillatorParams Params;
};

class FSequenceCameraShakePattern : public ICameraShakePattern
{
public:
    // Sequence에서는 TotalDuration 무시하고 Sequence 총 길이 쓸게요..
    // 혹은 호출자에서 TotalDuration에 Sequence길이 넣어주면 될듯
    FCameraShakePatternUpdateResult UpdatePattern(
        float DeltaTime,
        float ElapsedTime,
        float TotalDuration = -1.0f) override;

public:
    class UCameraAnimationSequence
    {

    };
    // TODO: 어흑마이깟
    UCameraAnimationSequence* Sequence = nullptr;
};
