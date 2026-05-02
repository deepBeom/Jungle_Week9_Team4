#pragma once
#include <unordered_set>

#include "Component/ShapeComponent.h"

class UWorld;

struct FCollisionPair
{
    UShapeComponent* A = nullptr;
    UShapeComponent* B = nullptr;

    FCollisionPair() = default;

    FCollisionPair(UShapeComponent* InA, UShapeComponent* InB)
    {
        if (InA < InB)
        {
            A = InA;
            B = InB;
        }
        else
        {
            A = InB;
            B = InA;
        }
    }

    bool operator==(const FCollisionPair& Other) const
    {
        return A == Other.A && B == Other.B;
    }
};

struct FCollisionPairHash
{
    size_t operator()(const FCollisionPair& Pair) const
    {
        const size_t A = reinterpret_cast<size_t>(Pair.A);
        const size_t B = reinterpret_cast<size_t>(Pair.B);
        return (A >> 4) ^ (B << 4);
    }
};

struct FCollisionDebugContact
{
    FVector Location = FVector::ZeroVector;
    FVector Normal = FVector::ZeroVector;
    FAABB OverlapBounds;
    bool bHasOverlapBounds = false;
    
    UShapeComponent* A = nullptr;
    UShapeComponent* B = nullptr;
};

class FCollisionSystem
{
public:
    void Tick(UWorld* World, float DeltaTime);
    
    const TArray<FCollisionDebugContact>& GetDebugContacts() const
    {
        return DebugContacts;
    }
    
private:
    void CollectShapeComponents(UWorld* World, TArray<UShapeComponent*>& OutShapes);
    
    bool ShouldTestPair(const UShapeComponent* A, const UShapeComponent* B) const;
    bool AreOverlapping(UShapeComponent* A, UShapeComponent* B) const;

    bool AABBOverlap(UShapeComponent* A, UShapeComponent* B) const;
    bool SphereSphere(UShapeComponent* A, UShapeComponent* B) const;
    bool SphereBox(UShapeComponent* Sphere, UShapeComponent* Box) const;
    bool BoxBox(UShapeComponent* A, UShapeComponent* B) const;
    bool CapsuleCapsule(UShapeComponent* A, UShapeComponent* B) const;
    bool SphereCapsule(UShapeComponent* Sphere, UShapeComponent* Capsule) const;
    bool BoxCapsule(UShapeComponent* Box, UShapeComponent* Capsule) const;

    void HandleBeginOverlap(UShapeComponent* A, UShapeComponent* B);
    void HandleEndOverlap(UShapeComponent* A, UShapeComponent* B);
    void HandleHit(UShapeComponent* A, UShapeComponent* B);
    void ResolveBlockingOverlap(UShapeComponent* A, UShapeComponent* B);
    
    void AddDebugContact(UShapeComponent* A, UShapeComponent* B);
    FVector ComputeDebugContactLocation(UShapeComponent* A, UShapeComponent* B) const;
    bool ComputeDebugOverlapBounds(UShapeComponent* A, UShapeComponent* B, FAABB& OutBounds) const;
    FCollisionEvent MakeCollisionEvent(UShapeComponent* Self, UShapeComponent* Other, bool bBlockingHit) const;
    void LogCollisionEvent(const char* EventName, const FCollisionEvent& Event) const;
    
private:
    TSet<FCollisionPair, FCollisionPairHash> PreviousOverlaps;
    TArray<FCollisionDebugContact> DebugContacts;
};
