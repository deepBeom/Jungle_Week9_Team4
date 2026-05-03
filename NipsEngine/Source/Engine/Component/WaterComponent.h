#pragma once

#include "Component/ActorComponent.h"

struct FWaterUniformData;
struct FOceanWaterProfile;

class UWaterComponent : public UActorComponent
{
public:
    DECLARE_CLASS(UWaterComponent, UActorComponent)

    UWaterComponent() = default;
    ~UWaterComponent() override = default;

    void Serialize(FArchive& Ar) override;
    void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
    void PostEditProperty(const char* PropertyName) override;

    void FillWaterUniformData(FWaterUniformData& OutData, float TimeSeconds, uint32 LocalLightCount) const;
    void ApplyOceanWaterProfile(const FOceanWaterProfile& InProfile);

private:
    // Stage 1 animated surface parameters.
    float NormalStrength = 0.45f;
    float Alpha = 1.0f;
    float ColorVariationStrength = 0.15f;

    float NormalTilingAX = 4.0f;
    float NormalTilingAY = 4.0f;
    float NormalScrollSpeedAX = 0.03f;
    float NormalScrollSpeedAY = 0.01f;

    float NormalTilingBX = 2.5f;
    float NormalTilingBY = 2.5f;
    float NormalScrollSpeedBX = -0.02f;
    float NormalScrollSpeedBY = 0.015f;
    float WorldUVScaleX = 0.02f;
    float WorldUVScaleY = 0.02f;
    float WorldUVBlendFactor = 1.0f;

    // Stage 2 highlight parameters.
    FColor BaseColor = FColor(0.08f, 0.22f, 0.33f, 1.0f);
    float WaterSpecularPower = 96.0f;
    float WaterSpecularIntensity = 0.75f;
    float WaterFresnelPower = 5.0f;
    float WaterFresnelIntensity = 0.45f;
    float WaterLightContributionScale = 1.0f;
    bool bEnableWaterSpecular = true;
};
