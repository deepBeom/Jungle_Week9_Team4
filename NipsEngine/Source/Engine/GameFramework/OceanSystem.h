#pragma once

#include "Core/Singleton.h"
#include "Math/Color.h"

class AActor;

struct FOceanWaterProfile
{
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

    FColor BaseColor = FColor(0.08f, 0.22f, 0.33f, 1.0f);
    float WaterSpecularPower = 96.0f;
    float WaterSpecularIntensity = 0.75f;
    float WaterFresnelPower = 5.0f;
    float WaterFresnelIntensity = 0.45f;
    float WaterLightContributionScale = 1.0f;
    bool bEnableWaterSpecular = true;
};

class FOceanSystem : public TSingleton<FOceanSystem>
{
    friend class TSingleton<FOceanSystem>;

public:
    const FOceanWaterProfile& GetWaterProfile() const { return WaterProfile; }
    void SetWaterProfile(const FOceanWaterProfile& InProfile);

    uint64 GetWaterProfileRevision() const { return WaterProfileRevision; }

    void RegisterGlobalOceanActor(const AActor* Actor);
    void UnregisterGlobalOceanActor(const AActor* Actor);
    uint32 GetRegisteredActorCount() const { return static_cast<uint32>(RegisteredOceanActors.size()); }

private:
    FOceanSystem() = default;
    ~FOceanSystem() = default;

private:
    FOceanWaterProfile WaterProfile = {};
    uint64 WaterProfileRevision = 1;
    TArray<const AActor*> RegisteredOceanActors;
};

