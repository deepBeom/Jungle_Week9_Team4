#include "WaterComponent.h"

#include "Object/ObjectFactory.h"
#include "Render/Scene/RenderCommand.h"

DEFINE_CLASS(UWaterComponent, UActorComponent)
REGISTER_FACTORY(UWaterComponent)

void UWaterComponent::Serialize(FArchive& Ar)
{
    UActorComponent::Serialize(Ar);

    Ar << "NormalStrength" << NormalStrength;
    Ar << "Alpha" << Alpha;
    Ar << "ColorVariationStrength" << ColorVariationStrength;

    Ar << "NormalTilingAX" << NormalTilingAX;
    Ar << "NormalTilingAY" << NormalTilingAY;
    Ar << "NormalScrollSpeedAX" << NormalScrollSpeedAX;
    Ar << "NormalScrollSpeedAY" << NormalScrollSpeedAY;

    Ar << "NormalTilingBX" << NormalTilingBX;
    Ar << "NormalTilingBY" << NormalTilingBY;
    Ar << "NormalScrollSpeedBX" << NormalScrollSpeedBX;
    Ar << "NormalScrollSpeedBY" << NormalScrollSpeedBY;

    Ar << "BaseColor" << BaseColor;
    Ar << "WaterSpecularPower" << WaterSpecularPower;
    Ar << "WaterSpecularIntensity" << WaterSpecularIntensity;
    Ar << "WaterFresnelPower" << WaterFresnelPower;
    Ar << "WaterFresnelIntensity" << WaterFresnelIntensity;
    Ar << "WaterLightContributionScale" << WaterLightContributionScale;
    Ar << "EnableWaterSpecular" << bEnableWaterSpecular;
}

void UWaterComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
    UActorComponent::GetEditableProperties(OutProps);

    OutProps.push_back({ "NormalStrength", EPropertyType::Float, &NormalStrength, 0.0f, 2.0f, 0.01f });
    OutProps.push_back({ "Alpha", EPropertyType::Float, &Alpha, 0.0f, 1.0f, 0.01f });
    OutProps.push_back({ "ColorVariationStrength", EPropertyType::Float, &ColorVariationStrength, 0.0f, 1.0f, 0.01f });

    OutProps.push_back({ "NormalTilingAX", EPropertyType::Float, &NormalTilingAX, 0.01f, 32.0f, 0.01f });
    OutProps.push_back({ "NormalTilingAY", EPropertyType::Float, &NormalTilingAY, 0.01f, 32.0f, 0.01f });
    OutProps.push_back({ "NormalScrollSpeedAX", EPropertyType::Float, &NormalScrollSpeedAX, -5.0f, 5.0f, 0.001f });
    OutProps.push_back({ "NormalScrollSpeedAY", EPropertyType::Float, &NormalScrollSpeedAY, -5.0f, 5.0f, 0.001f });

    OutProps.push_back({ "NormalTilingBX", EPropertyType::Float, &NormalTilingBX, 0.01f, 32.0f, 0.01f });
    OutProps.push_back({ "NormalTilingBY", EPropertyType::Float, &NormalTilingBY, 0.01f, 32.0f, 0.01f });
    OutProps.push_back({ "NormalScrollSpeedBX", EPropertyType::Float, &NormalScrollSpeedBX, -5.0f, 5.0f, 0.001f });
    OutProps.push_back({ "NormalScrollSpeedBY", EPropertyType::Float, &NormalScrollSpeedBY, -5.0f, 5.0f, 0.001f });

    OutProps.push_back({ "BaseColor", EPropertyType::Color, &BaseColor });
    OutProps.push_back({ "WaterSpecularPower", EPropertyType::Float, &WaterSpecularPower, 1.0f, 512.0f, 1.0f });
    OutProps.push_back({ "WaterSpecularIntensity", EPropertyType::Float, &WaterSpecularIntensity, 0.0f, 10.0f, 0.01f });
    OutProps.push_back({ "WaterFresnelPower", EPropertyType::Float, &WaterFresnelPower, 1.0f, 16.0f, 0.1f });
    OutProps.push_back({ "WaterFresnelIntensity", EPropertyType::Float, &WaterFresnelIntensity, 0.0f, 10.0f, 0.01f });
    OutProps.push_back({ "WaterLightContributionScale", EPropertyType::Float, &WaterLightContributionScale, 0.0f, 10.0f, 0.01f });
    OutProps.push_back({ "EnableWaterSpecular", EPropertyType::Bool, &bEnableWaterSpecular });
}

void UWaterComponent::PostEditProperty(const char* PropertyName)
{
    UActorComponent::PostEditProperty(PropertyName);
    (void)PropertyName;
}

void UWaterComponent::FillWaterUniformData(FWaterUniformData& OutData, float TimeSeconds, uint32 LocalLightCount) const
{
    OutData.Time = TimeSeconds;
    OutData.NormalStrength = NormalStrength;
    OutData.Alpha = Alpha;
    OutData.WaterSpecularPower = WaterSpecularPower;

    OutData.NormalTilingA = FVector2(NormalTilingAX, NormalTilingAY);
    OutData.NormalScrollSpeedA = FVector2(NormalScrollSpeedAX, NormalScrollSpeedAY);
    OutData.NormalTilingB = FVector2(NormalTilingBX, NormalTilingBY);
    OutData.NormalScrollSpeedB = FVector2(NormalScrollSpeedBX, NormalScrollSpeedBY);

    OutData.BaseColor = FVector(BaseColor.R, BaseColor.G, BaseColor.B);
    OutData.WaterSpecularIntensity = WaterSpecularIntensity;
    OutData.ColorVariationStrength = ColorVariationStrength;
    OutData.WaterFresnelPower = WaterFresnelPower;
    OutData.WaterFresnelIntensity = WaterFresnelIntensity;
    OutData.WaterLightContributionScale = WaterLightContributionScale;
    OutData.bEnableWaterSpecular = bEnableWaterSpecular ? 1u : 0u;
    OutData.WaterLocalLightCount = LocalLightCount;
}
