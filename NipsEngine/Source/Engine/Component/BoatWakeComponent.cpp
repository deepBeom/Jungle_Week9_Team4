#include "BoatWakeComponent.h"

#include "Component/DecalComponent.h"
#include "Component/SceneComponent.h"
#include "Core/ResourceManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PrimitiveActors.h"
#include "GameFramework/World.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"
#include "Math/Utils.h"
#include "Object/ObjectFactory.h"

#include <cmath>

namespace
{
    constexpr float WakeMinDistanceEpsilon = 1.0e-4f;
    constexpr float WakeMinAxisEpsilon = 1.0e-3f;

    float Clamp01(float Value)
    {
        return MathUtil::Clamp(Value, 0.0f, 1.0f);
    }

    float Lerp(float A, float B, float Alpha)
    {
        return A + (B - A) * Clamp01(Alpha);
    }

    FVector MakePlanarVector(const FVector& Value)
    {
        return FVector(Value.X, Value.Y, 0.0f);
    }
}

DEFINE_CLASS(UBoatWakeComponent, UActorComponent)
REGISTER_FACTORY(UBoatWakeComponent)

void UBoatWakeComponent::Serialize(FArchive& Ar)
{
    UActorComponent::Serialize(Ar);

    Ar << "Main Decal Material" << MainDecalMaterial;
    Ar << "Variant Decal Material" << VariantDecalMaterial;
    Ar << "Min Spawn Speed" << MinSpawnSpeed;
    Ar << "Max Wake Speed" << MaxWakeSpeed;
    Ar << "Spawn Spacing" << SpawnSpacing;

    Ar << "Main Width" << MainWidth;
    Ar << "Main Length" << MainLength;
    Ar << "Main Depth" << MainDepth;
    Ar << "Main Backward Offset" << MainBackwardOffset;
    Ar << "Main Fade Start Delay" << MainFadeStartDelay;
    Ar << "Main Fade Duration" << MainFadeDuration;

    Ar << "Variant Width" << VariantWidth;
    Ar << "Variant Length" << VariantLength;
    Ar << "Variant Depth" << VariantDepth;
    Ar << "Variant Side Offset" << VariantSideOffset;
    Ar << "Variant Backward Offset" << VariantBackwardOffset;
    Ar << "Variant Turn Angle Degrees" << VariantTurnAngleDegrees;
    Ar << "Variant Fade Start Delay" << VariantFadeStartDelay;
    Ar << "Variant Fade Duration" << VariantFadeDuration;
    Ar << "Turn Threshold" << TurnThreshold;

    Ar << "Water Height Offset" << WaterHeightOffset;

    if (Ar.IsLoading())
    {
        MainDecalMaterialRef = nullptr;
        VariantDecalMaterialRef = nullptr;
        DistanceAccumulator = 0.0f;
        bHasHistory = false;
        LastOwnerLocation = FVector::ZeroVector;
        LastBoatForward = FVector::ForwardVector;
    }
}

void UBoatWakeComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
    UActorComponent::GetEditableProperties(OutProps);

    OutProps.push_back({ "Main Decal Material", EPropertyType::String, &MainDecalMaterial });
    OutProps.push_back({ "Variant Decal Material", EPropertyType::String, &VariantDecalMaterial });
    OutProps.push_back({ "Min Spawn Speed", EPropertyType::Float, &MinSpawnSpeed, 0.0f, 20.0f, 0.05f });
    OutProps.push_back({ "Max Wake Speed", EPropertyType::Float, &MaxWakeSpeed, 0.1f, 40.0f, 0.05f });
    OutProps.push_back({ "Spawn Spacing", EPropertyType::Float, &SpawnSpacing, 0.2f, 20.0f, 0.05f });

    OutProps.push_back({ "Main Width", EPropertyType::Float, &MainWidth, 0.1f, 30.0f, 0.05f });
    OutProps.push_back({ "Main Length", EPropertyType::Float, &MainLength, 0.1f, 40.0f, 0.05f });
    OutProps.push_back({ "Main Depth", EPropertyType::Float, &MainDepth, 0.1f, 10.0f, 0.05f });
    OutProps.push_back({ "Main Backward Offset", EPropertyType::Float, &MainBackwardOffset, 0.0f, 20.0f, 0.05f });
    OutProps.push_back({ "Main Fade Start Delay", EPropertyType::Float, &MainFadeStartDelay, 0.0f, 5.0f, 0.01f });
    OutProps.push_back({ "Main Fade Duration", EPropertyType::Float, &MainFadeDuration, 0.05f, 8.0f, 0.01f });

    OutProps.push_back({ "Variant Width", EPropertyType::Float, &VariantWidth, 0.1f, 30.0f, 0.05f });
    OutProps.push_back({ "Variant Length", EPropertyType::Float, &VariantLength, 0.1f, 30.0f, 0.05f });
    OutProps.push_back({ "Variant Depth", EPropertyType::Float, &VariantDepth, 0.1f, 10.0f, 0.05f });
    OutProps.push_back({ "Variant Side Offset", EPropertyType::Float, &VariantSideOffset, 0.0f, 20.0f, 0.05f });
    OutProps.push_back({ "Variant Backward Offset", EPropertyType::Float, &VariantBackwardOffset, 0.0f, 20.0f, 0.05f });
    OutProps.push_back({ "Variant Turn Angle Degrees", EPropertyType::Float, &VariantTurnAngleDegrees, 0.0f, 90.0f, 0.1f });
    OutProps.push_back({ "Variant Fade Start Delay", EPropertyType::Float, &VariantFadeStartDelay, 0.0f, 5.0f, 0.01f });
    OutProps.push_back({ "Variant Fade Duration", EPropertyType::Float, &VariantFadeDuration, 0.05f, 8.0f, 0.01f });
    OutProps.push_back({ "Turn Threshold", EPropertyType::Float, &TurnThreshold, 0.0f, 1.0f, 0.01f });
    OutProps.push_back({ "Water Height Offset", EPropertyType::Float, &WaterHeightOffset, -2.0f, 2.0f, 0.01f });
}

void UBoatWakeComponent::PostEditProperty(const char* PropertyName)
{
    UActorComponent::PostEditProperty(PropertyName);
    (void)PropertyName;
    RefreshMaterialRefs();
}

void UBoatWakeComponent::BeginPlay()
{
    UActorComponent::BeginPlay();

    RefreshMaterialRefs();

    if (AActor* Owner = GetOwner())
    {
        LastOwnerLocation = Owner->GetActorLocation();

        FVector BoatForward;
        FVector BoatRight;
        if (ResolveBoatAxes(BoatForward, BoatRight))
        {
            LastBoatForward = BoatForward;
        }

        bHasHistory = true;
    }
}

void UBoatWakeComponent::TickComponent(float DeltaTime)
{
    if (DeltaTime <= 0.0f)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (Owner == nullptr || Owner->IsPendingDestroy())
    {
        return;
    }

    if (MainDecalMaterialRef == nullptr || VariantDecalMaterialRef == nullptr)
    {
        RefreshMaterialRefs();
    }

    FVector BoatForward;
    FVector BoatRight;
    if (!ResolveBoatAxes(BoatForward, BoatRight))
    {
        return;
    }

    const FVector CurrentLocation = Owner->GetActorLocation();
    const FVector MovementDelta = MakePlanarVector(CurrentLocation - LastOwnerLocation);
    const float MovedDistance = MovementDelta.Size();

    if (!bHasHistory)
    {
        LastOwnerLocation = CurrentLocation;
        LastBoatForward = BoatForward;
        bHasHistory = true;
        return;
    }

    if (MovedDistance <= WakeMinDistanceEpsilon)
    {
        LastOwnerLocation = CurrentLocation;
        LastBoatForward = BoatForward;
        return;
    }

    const FVector MoveDir = MovementDelta / MovedDistance;
    const float Speed = MovedDistance / DeltaTime;
    const float SpeedRange = (MaxWakeSpeed - MinSpawnSpeed) > 0.001f ? (MaxWakeSpeed - MinSpawnSpeed) : 0.001f;
    const float SpeedAlpha = Clamp01((Speed - MinSpawnSpeed) / SpeedRange);

    const FVector LastForwardPlanar = MakePlanarVector(LastBoatForward).GetSafeNormal2D();
    const FVector CurrentForwardPlanar = MakePlanarVector(BoatForward).GetSafeNormal2D();

    const float SignedTurn = FVector::DotProduct(MoveDir, BoatRight);
    float TurnAlpha = MathUtil::Abs(SignedTurn);
    if (!LastForwardPlanar.IsNearlyZero() && !CurrentForwardPlanar.IsNearlyZero())
    {
        const float HeadingDot = MathUtil::Clamp(FVector::DotProduct(LastForwardPlanar, CurrentForwardPlanar), -1.0f, 1.0f);
        const float HeadingDeltaRadians = std::acos(HeadingDot);
        TurnAlpha = (TurnAlpha > Clamp01(MathUtil::RadiansToDegrees(HeadingDeltaRadians) / 20.0f))
            ? TurnAlpha
            : Clamp01(MathUtil::RadiansToDegrees(HeadingDeltaRadians) / 20.0f);
    }

    if (Speed >= MinSpawnSpeed)
    {
        DistanceAccumulator += MovedDistance;

        while (DistanceAccumulator >= SpawnSpacing)
        {
            const float Overshoot = DistanceAccumulator - SpawnSpacing;
            const FVector SampleLocation = CurrentLocation - MoveDir * Overshoot;
            SpawnWakeSet(SampleLocation, BoatForward, BoatRight, MoveDir, SpeedAlpha, TurnAlpha, SignedTurn);
            DistanceAccumulator -= SpawnSpacing;
        }
    }
    else
    {
        DistanceAccumulator = 0.0f;
    }

    LastOwnerLocation = CurrentLocation;
    LastBoatForward = BoatForward;
}

void UBoatWakeComponent::RefreshMaterialRefs()
{
    MainDecalMaterialRef = MainDecalMaterial.empty()
        ? nullptr
        : FResourceManager::Get().GetMaterialInterface(MainDecalMaterial);
    VariantDecalMaterialRef = VariantDecalMaterial.empty()
        ? nullptr
        : FResourceManager::Get().GetMaterialInterface(VariantDecalMaterial);
}

bool UBoatWakeComponent::ResolveBoatAxes(FVector& OutForward, FVector& OutRight) const
{
    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        return false;
    }

    if (APawn* Pawn = Cast<APawn>(Owner))
    {
        OutForward = MakePlanarVector(Pawn->GetForwardVector()).GetSafeNormal2D();
        OutRight = MakePlanarVector(Pawn->GetRightVector()).GetSafeNormal2D();
    }
    else
    {
        OutForward = MakePlanarVector(Owner->GetActorForward()).GetSafeNormal2D();
        if (USceneComponent* Root = Owner->GetRootComponent())
        {
            OutRight = MakePlanarVector(Root->GetRightVector()).GetSafeNormal2D();
        }
        else
        {
            OutRight = FVector::RightVector;
        }
    }

    if (OutForward.SizeSquared() <= WakeMinAxisEpsilon)
    {
        return false;
    }

    if (OutRight.SizeSquared() <= WakeMinAxisEpsilon)
    {
        OutRight = FVector::CrossProduct(FVector::UpVector, OutForward).GetSafeNormal2D();
    }

    return OutRight.SizeSquared() > WakeMinAxisEpsilon;
}

void UBoatWakeComponent::SpawnWakeSet(
    const FVector& BoatLocation,
    const FVector& BoatForward,
    const FVector& BoatRight,
    const FVector& MoveDir,
    float SpeedAlpha,
    float TurnAlpha,
    float SignedTurn)
{
    if (MainDecalMaterialRef == nullptr && VariantDecalMaterialRef == nullptr)
    {
        return;
    }

    const FVector WaterOffset(0.0f, 0.0f, WaterHeightOffset);
    const float MainWidthScale = Lerp(0.85f, 1.15f, SpeedAlpha);
    const float MainLengthScale = Lerp(0.80f, 1.20f, SpeedAlpha);
    const FVector MainSize(
        MainDepth,
        MainWidth * MainWidthScale,
        MainLength * MainLengthScale);

    const FVector SternAnchor = BoatLocation + WaterOffset;
    const FVector MainLocation =
        SternAnchor -
        BoatForward * MainBackwardOffset;

    if (MainDecalMaterialRef != nullptr)
    {
        SpawnWakeDecal(
            MainLocation,
            BoatForward,
            BoatRight,
            MainSize,
            MainDecalMaterialRef,
            MainFadeStartDelay,
            MainFadeDuration);
    }

    if (VariantDecalMaterialRef == nullptr)
    {
        return;
    }

    const float TurnRange = (1.0f - TurnThreshold) > 0.001f ? (1.0f - TurnThreshold) : 0.001f;
    const float VariantBlend = Clamp01((TurnAlpha - TurnThreshold) / TurnRange);
    if (VariantBlend <= 0.0f)
    {
        return;
    }

    const float TurnSide = SignedTurn >= 0.0f ? 1.0f : -1.0f;
    const FVector VariantBaseForward = MoveDir.SizeSquared() > WakeMinAxisEpsilon ? MoveDir : BoatForward;
    const FQuat VariantYaw(FVector::UpVector, MathUtil::DegreesToRadians(TurnSide * VariantTurnAngleDegrees));
    const FVector VariantForward = (VariantYaw * VariantBaseForward).GetSafeNormal2D();
    const FVector VariantRight = FVector::CrossProduct(FVector::UpVector, VariantForward).GetSafeNormal2D();

    const FVector VariantSize(
        VariantDepth,
        VariantWidth * Lerp(0.9f, 1.1f, VariantBlend),
        VariantLength * Lerp(0.8f, 1.25f, VariantBlend));

    const FVector VariantLocation =
        SternAnchor +
        BoatRight * (TurnSide * VariantSideOffset) -
        BoatForward * VariantBackwardOffset;

    SpawnWakeDecal(
        VariantLocation,
        VariantForward,
        VariantRight,
        VariantSize,
        VariantDecalMaterialRef,
        VariantFadeStartDelay,
        VariantFadeDuration);
}

void UBoatWakeComponent::SpawnWakeDecal(
    const FVector& WorldLocation,
    const FVector& PlaneForward,
    const FVector& PlaneRight,
    const FVector& DecalSize,
    UMaterialInterface* Material,
    float FadeStartDelay,
    float FadeDuration) const
{
    AActor* Owner = GetOwner();
    UWorld* World = Owner ? Owner->GetFocusedWorld() : nullptr;
    if (World == nullptr || Material == nullptr)
    {
        return;
    }

    ADecalActor* DecalActor = World->SpawnActor<ADecalActor>();
    if (DecalActor == nullptr)
    {
        return;
    }

    UDecalComponent* DecalComponent = Cast<UDecalComponent>(DecalActor->GetRootComponent());
    if (DecalComponent == nullptr)
    {
        World->DestroyActor(DecalActor);
        return;
    }

    DecalActor->SetActorLocation(WorldLocation);
    DecalComponent->SetSize(DecalSize);
    DecalComponent->SetMaterial(Material);
    DecalComponent->SetFadeIn(0.0f, 0.08f);
    DecalComponent->SetFadeOut(FadeStartDelay, FadeDuration, true);

    const FVector ProjectionAxis = FVector::UpVector;
    const FVector DecalForward = MakePlanarVector(PlaneForward).GetSafeNormal2D();
    const FVector DecalRight = MakePlanarVector(PlaneRight).GetSafeNormal2D();
    if (DecalForward.SizeSquared() <= WakeMinAxisEpsilon || DecalRight.SizeSquared() <= WakeMinAxisEpsilon)
    {
        World->DestroyActor(DecalActor);
        return;
    }

    FMatrix RotationMatrix = FMatrix::Identity;
    RotationMatrix.SetAxes(ProjectionAxis, DecalRight, DecalForward);

    if (USceneComponent* RootComponent = DecalActor->GetRootComponent())
    {
        FQuat RotationQuat(RotationMatrix);
        RotationQuat.Normalize();
        RootComponent->SetRelativeRotationQuat(RotationQuat);
    }
}
