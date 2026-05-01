#include "ShapeComponent.h"

#include <algorithm>
#include <cmath>

#include "GameFramework/AActor.h"

DEFINE_CLASS(UShapeComponent, UPrimitiveComponent)

DEFINE_CLASS(UBoxComponent, UShapeComponent)
REGISTER_FACTORY(UBoxComponent)

DEFINE_CLASS(USphereComponent, UShapeComponent)
REGISTER_FACTORY(USphereComponent)

DEFINE_CLASS(UCapsuleComponent, UShapeComponent)
REGISTER_FACTORY(UCapsuleComponent)

namespace
{
    float MaxAbs3(const FVector& V)
    {
        return std::max({ std::fabs(V.X), std::fabs(V.Y), std::fabs(V.Z) });
    }

    float CapsuleRadiusScale(const FVector& Scale)
    {
        return std::max(std::fabs(Scale.X), std::fabs(Scale.Y));
    }

    float CapsuleHeightScale(const FVector& Scale)
    {
        return std::fabs(Scale.Z);
    }

    FVector AbsVector(const FVector& V)
    {
        return FVector(std::fabs(V.X), std::fabs(V.Y), std::fabs(V.Z));
    }
}

void UShapeComponent::Serialize(FArchive& Ar)
{
    UPrimitiveComponent::Serialize(Ar);

    Ar << "GenerateOverlapEvents" << bGenerateOverlapEvents;
    Ar << "BlockComponent" << bBlockComponent;
    Ar << "ShapeColor" << ShapeColor;
    Ar << "DrawOnlyIfSelected" << bDrawOnlyIfSelected;
}

void UShapeComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
    UPrimitiveComponent::GetEditableProperties(OutProps);

    OutProps.push_back({ "Generate Overlap Events", EPropertyType::Bool, &bGenerateOverlapEvents });
    OutProps.push_back({ "Block Component", EPropertyType::Bool, &bBlockComponent });
    OutProps.push_back({ "Shape Color", EPropertyType::Color, &ShapeColor });
    OutProps.push_back({ "Draw Only If Selected", EPropertyType::Bool, &bDrawOnlyIfSelected });
}

void UShapeComponent::PostEditProperty(const char* PropertyName)
{
    UPrimitiveComponent::PostEditProperty(PropertyName);
    NotifySpatialIndexDirty();
}

bool UShapeComponent::IsOverlappingActor(const AActor* Other) const
{
    if (!Other)
    {
        return false;
    }
    
    for (const FOverlapInfo& Info : OverlapInfos)
    {
        if (Info.OverlapActor == Other)
        {
            return true;
        }
    }
    
    return false;
}

void UShapeComponent::AddOverlap(UPrimitiveComponent* OtherComponent)
{
    if (!OtherComponent)
    {
        return;
    }
    
    AActor* OtherActor = OtherComponent->GetOwner();
    if (!OtherActor)
    {
        return;
    }
    for (const FOverlapInfo& Info : OverlapInfos)
    {
        if (Info.OverlapComponent == OtherComponent)
        {
            return;
        }
    }
    
    FOverlapInfo NewInfo;
    NewInfo.OverlapComponent = OtherComponent;
    NewInfo.OverlapActor = OtherActor;
    OverlapInfos.push_back(NewInfo);
}

void UShapeComponent::RemoveOverlap(UPrimitiveComponent* OtherComponent)
{
    if (!OtherComponent)
    {
        return;
    }

    for (auto It = OverlapInfos.begin(); It != OverlapInfos.end(); ++It)
    {
        if (It->OverlapComponent == OtherComponent)
        {
            OverlapInfos.erase(It);
            return;
        }
    }
}

void UShapeComponent::ClearOverlapInfos()
{
    OverlapInfos.clear();
}

void UShapeComponent::DispatchBeginOverlap(const FCollisionEvent& Event)
{
    if (OnComponentBeginOverlap)
    {
        OnComponentBeginOverlap(Event); // OnComponentBeginOverlap.Broadcast(Event);
    }
}

void UShapeComponent::DispatchEndOverlap(const FCollisionEvent& Event)
{
    if (OnComponentEndOverlap)
    {
        OnComponentEndOverlap(Event);
    }
}

void UShapeComponent::DispatchHit(const FCollisionEvent& Event)
{
    if (OnComponentHit)
    {
        OnComponentHit(Event);
    }
}

// --- Box ---
void UBoxComponent::Serialize(FArchive& Ar)
{
    UShapeComponent::Serialize(Ar);
    Ar << "BoxExtent" << BoxExtent;
}

void UBoxComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
    UShapeComponent::GetEditableProperties(OutProps);
    OutProps.push_back({ "Box Extent", EPropertyType::Vec3, &BoxExtent });
}

void UBoxComponent::PostEditProperty(const char* PropertyName)
{
    UShapeComponent::PostEditProperty(PropertyName);
    NotifySpatialIndexDirty();
}

void UBoxComponent::UpdateWorldAABB() const
{
    const FAABB LocalAABB(-BoxExtent, BoxExtent);
    WorldAABB = FAABB::TransformAABB(LocalAABB, GetWorldMatrix());
}

// --- Sphere ---
void USphereComponent::Serialize(FArchive& Ar)
{
    UShapeComponent::Serialize(Ar);
    Ar << "SphereRadius" << SphereRadius;
}

void USphereComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
    UShapeComponent::GetEditableProperties(OutProps);

    FPropertyDescriptor RadiusProp;
    RadiusProp.Name = "Sphere Radius";
    RadiusProp.Type = EPropertyType::Float;
    RadiusProp.ValuePtr = &SphereRadius;
    RadiusProp.Min = 0.0f;
    RadiusProp.Max = 100000.0f;
    RadiusProp.Speed = 1.0f;
    OutProps.push_back(RadiusProp);
}

void USphereComponent::PostEditProperty(const char* PropertyName)
{
    UShapeComponent::PostEditProperty(PropertyName);

    if (SphereRadius < 0.0f)
    {
        SphereRadius = 0.0f;
    }

    NotifySpatialIndexDirty();
}

void USphereComponent::UpdateWorldAABB() const
{
    const FVector Center = GetWorldLocation();
    const float WorldRadius = SphereRadius * MaxAbs3(GetWorldScale());
    const FVector Extent(WorldRadius, WorldRadius, WorldRadius);
    
    WorldAABB = FAABB(Center - Extent, Center + Extent);
}

// --- Capsule ---
void UCapsuleComponent::Serialize(FArchive& Ar)
{
    UShapeComponent::Serialize(Ar);
    Ar << "CapsuleHalfHeight" << CapsuleHalfHeight;
    Ar << "CapsuleRadius" << CapsuleRadius;
}

void UCapsuleComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
    UShapeComponent::GetEditableProperties(OutProps);

    FPropertyDescriptor HalfHeightProp;
    HalfHeightProp.Name = "Capsule Half Height";
    HalfHeightProp.Type = EPropertyType::Float;
    HalfHeightProp.ValuePtr = &CapsuleHalfHeight;
    HalfHeightProp.Min = 0.0f;
    HalfHeightProp.Max = 100000.0f;
    HalfHeightProp.Speed = 1.0f;
    OutProps.push_back(HalfHeightProp);

    FPropertyDescriptor RadiusProp;
    RadiusProp.Name = "Capsule Radius";
    RadiusProp.Type = EPropertyType::Float;
    RadiusProp.ValuePtr = &CapsuleRadius;
    RadiusProp.Min = 0.0f;
    RadiusProp.Max = 100000.0f;
    RadiusProp.Speed = 1.0f;
    OutProps.push_back(RadiusProp);
}

void UCapsuleComponent::PostEditProperty(const char* PropertyName)
{
    UShapeComponent::PostEditProperty(PropertyName);

    if (CapsuleRadius < 0.0f)
    {
        CapsuleRadius = 0.0f;
    }

    if (CapsuleHalfHeight < CapsuleRadius)
    {
        CapsuleHalfHeight = CapsuleRadius;
    }

    NotifySpatialIndexDirty();
}

void UCapsuleComponent::UpdateWorldAABB() const
{
    const FVector WorldScale = GetWorldScale();
    const float WorldRadius = CapsuleRadius * CapsuleRadiusScale(WorldScale);
    const float WorldHalfHeight = std::max(CapsuleHalfHeight * CapsuleHeightScale(WorldScale), WorldRadius);
    const float SegmentHalfHeight = std::max(0.0f, WorldHalfHeight - WorldRadius);

    const FVector Center = GetWorldLocation();
    const FVector Forward = GetForwardVector();
    const FVector Right = GetRightVector();
    const FVector Up = GetUpVector();

    const FVector Extent =
        AbsVector(Forward) * WorldRadius +
        AbsVector(Right) * WorldRadius +
        AbsVector(Up) * (SegmentHalfHeight + WorldRadius);

    WorldAABB = FAABB(Center - Extent, Center + Extent);
}
