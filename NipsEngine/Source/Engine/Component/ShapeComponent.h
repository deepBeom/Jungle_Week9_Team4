#pragma once
#include <functional>

#include "Core/Containers/String.h"
#include "PrimitiveComponent.h"
#include "Engine/Core/Delegate/DelegateMacros.h"

class AActor;
class UShapeComponent;
class UStaticMeshComponent;

struct FOverlapInfo
{
    UPrimitiveComponent* OverlapComponent = nullptr;
    AActor* OverlapActor = nullptr;
    
    bool IsValid() const
    {
        return OverlapComponent != nullptr && OverlapActor != nullptr;
    }
};

struct FCollisionEvent
{
    UShapeComponent* SelfComponent = nullptr;
    AActor* SelfActor = nullptr;
    UShapeComponent* OtherComponent = nullptr;
    AActor* OtherActor = nullptr;

    FString SelfTag = "Untagged";
    FString OtherTag = "Untagged";

    FVector Location = FVector::ZeroVector;
    FVector Normal = FVector::ZeroVector;
    FAABB OverlapBounds;
    bool bHasOverlapBounds = false;
    bool bBlockingHit = false;

    bool SelfHasTag(const FString& Tag) const
    {
        return SelfTag == Tag;
    }

    bool OtherHasTag(const FString& Tag) const
    {
        return OtherTag == Tag;
    }
};

using FCollisionEventCallback = std::function<void(const FCollisionEvent&)>;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHitDelegate, const FCollisionEvent&);
class UShapeComponent : public UPrimitiveComponent
{
public:
    DECLARE_CLASS(UShapeComponent, UPrimitiveComponent);
    
    UShapeComponent() = default;
    
    void Serialize(FArchive& Ar) override;
    void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
    void PostEditProperty(const char* PropertyName) override;

    bool FitToStaticMesh(UStaticMeshComponent* StaticMeshComponent);

    EPrimitiveType GetPrimitiveType() const override
    {
        return EPrimitiveType::EPT_CollisionShape;
    }
    
    bool RaycastMesh(const FRay& Ray, FHitResult& OutHitResult) override
    {
        (void)Ray;
        OutHitResult.Reset();
        return false;
    }
    
    bool GetGenerateOverlapEvents() const {return bGenerateOverlapEvents;}
    void SetGenerateOverlapEvents(bool bInGenerate) {bGenerateOverlapEvents = bInGenerate;}
    
    bool GetBlockComponent() const {return bBlockComponent;}
    void SetBlockComponent(bool bInBlock) {bBlockComponent = bInBlock;}

    const FColor& GetShapeColor() const { return ShapeColor; }
    void SetShapeColor(const FColor& InColor) { ShapeColor = InColor; }
    
    const TArray<FOverlapInfo>& GetOverlapInfos() const { return OverlapInfos; } 
    
    bool IsOverlappingActor(const AActor* Actor) const;
    void AddOverlap(UPrimitiveComponent* OverlapComponent);
    void RemoveOverlap(UPrimitiveComponent* OverlapComponent);
    void ClearOverlapInfos();

    void DispatchBeginOverlap(const FCollisionEvent& Event);
    void DispatchEndOverlap(const FCollisionEvent& Event);
    void DispatchHit(const FCollisionEvent& Event);

    FOnHitDelegate OnHit;
    FOnHitDelegate OnComponentBeginOverlap;
    FOnHitDelegate OnComponentEndOverlap;
    FOnHitDelegate OnComponentHit;
    
protected:
    bool bGenerateOverlapEvents = true;
    bool bBlockComponent = false;
    
    FColor ShapeColor = FColor::Green();
    
    TArray<FOverlapInfo> OverlapInfos;
};

class UBoxComponent : public UShapeComponent
{
public:;
    DECLARE_CLASS(UBoxComponent, UShapeComponent);
 
    UBoxComponent() = default;
    
    void Serialize(FArchive& Ar) override;
    void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
    void PostEditProperty(const char* PropertyName) override;
    
    void UpdateWorldAABB() const override;
    
    const FVector& GetBoxExtent() const {return BoxExtent;}
    void SetBoxExtent(const FVector& InExtent)
    {
        BoxExtent = InExtent;
        NotifySpatialIndexDirty();
    }
    
private:
    FVector BoxExtent = FVector{1.0f, 1.0f, 1.0f};
};

class USphereComponent : public UShapeComponent
{
public:;
    DECLARE_CLASS(USphereComponent, UShapeComponent);
    
    void Serialize(FArchive& Ar) override;
    void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
    void PostEditProperty(const char* PropertyName) override;

    void UpdateWorldAABB() const override;

    float GetSphereRadius() const { return SphereRadius; }
    void SetSphereRadius(float InRadius)
    {
        SphereRadius = InRadius;
        NotifySpatialIndexDirty();
    }
    
private:
    float SphereRadius = 1.0f;
};

class UCapsuleComponent : public UShapeComponent
{
public:;
    DECLARE_CLASS(UCapsuleComponent, UShapeComponent);
    
    UCapsuleComponent() = default;
    
    void Serialize(FArchive& Ar) override;
    void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
    void PostEditProperty(const char* PropertyName) override;

    void UpdateWorldAABB() const override;
    
    float GetCapsuleHalfHeight() const { return CapsuleHalfHeight; }
    float GetCapsuleRadius() const { return CapsuleRadius; }

    void SetCapsuleSize(float InRadius, float InHalfHeight)
    {
        CapsuleRadius = InRadius;
        CapsuleHalfHeight = InHalfHeight;
        NotifySpatialIndexDirty();
    }
    
private:
    float CapsuleHalfHeight = 1.0f;
    float CapsuleRadius = 0.5f;
};
