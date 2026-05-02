#include "CollisionSystem.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Component/ObjectTypeComponent.h"
#include "Component/Movement/MovementComponent.h"
#include "Core/Logging/Log.h"
#include "GameFramework/Actor.h"
#include "GameFramework/World.h"

namespace
{
    struct FOrientedBoxData
    {
        FVector Center = FVector::ZeroVector;
        FVector Axis[3] = { FVector::ForwardVector, FVector::RightVector, FVector::UpVector };
        float Extent[3] = { 0.0f, 0.0f, 0.0f };
    };

    struct FCapsuleData
    {
        FVector SegmentStart = FVector::ZeroVector;
        FVector SegmentEnd = FVector::ZeroVector;
        FVector Up = FVector::UpVector;
        float Radius = 0.0f;
        float HalfHeight = 0.0f;
        float SegmentHalfHeight = 0.0f;
    };

    const UObjectTypeComponent* FindObjectTypeComponent(const AActor* Actor)
    {
        if (!Actor)
        {
            return nullptr;
        }

        for (UActorComponent* Component : Actor->GetComponents())
        {
            if (const UObjectTypeComponent* ObjectType = Cast<UObjectTypeComponent>(Component))
            {
                return ObjectType;
            }
        }

        return nullptr;
    }

    const char* FindScriptPath(EObjectType Type)
    {
        const FObjectTypeBinding* Binding = FindObjectTypeBinding(Type);
        return Binding ? Binding->LuePath : "";
    }

    bool HasMovementComponent(const AActor* Actor)
    {
        if (!Actor)
        {
            return false;
        }

        for (UActorComponent* Component : Actor->GetComponents())
        {
            if (Cast<UMovementComponent>(Component))
            {
                return true;
            }
        }

        return false;
    }

    float Clamp(float Value, float Min, float Max)
    {
        return std::max(Min, std::min(Value, Max));
    }

    bool AABBIntersects(const FAABB& A, const FAABB& B)
    {
        return A.Min.X <= B.Max.X && A.Max.X >= B.Min.X &&
               A.Min.Y <= B.Max.Y && A.Max.Y >= B.Min.Y &&
               A.Min.Z <= B.Max.Z && A.Max.Z >= B.Min.Z;
    }

    FVector ComputeAABBDepenetration(const FAABB& MovingBox, const FAABB& BlockingBox)
    {
        if (!AABBIntersects(MovingBox, BlockingBox))
        {
            return FVector::ZeroVector;
        }

        const FVector MovingCenter = MovingBox.GetCenter();
        const FVector BlockingCenter = BlockingBox.GetCenter();

        const float PushX = std::min(MovingBox.Max.X - BlockingBox.Min.X, BlockingBox.Max.X - MovingBox.Min.X);
        const float PushY = std::min(MovingBox.Max.Y - BlockingBox.Min.Y, BlockingBox.Max.Y - MovingBox.Min.Y);
        const float PushZ = std::min(MovingBox.Max.Z - BlockingBox.Min.Z, BlockingBox.Max.Z - MovingBox.Min.Z);

        constexpr float Skin = 0.1f;

        if (PushX <= PushY && PushX <= PushZ)
        {
            return FVector((MovingCenter.X >= BlockingCenter.X ? 1.0f : -1.0f) * (PushX + Skin), 0.0f, 0.0f);
        }

        if (PushY <= PushX && PushY <= PushZ)
        {
            return FVector(0.0f, (MovingCenter.Y >= BlockingCenter.Y ? 1.0f : -1.0f) * (PushY + Skin), 0.0f);
        }

        return FVector(0.0f, 0.0f, (MovingCenter.Z >= BlockingCenter.Z ? 1.0f : -1.0f) * (PushZ + Skin));
    }

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

    FOrientedBoxData BuildOrientedBoxData(const UBoxComponent* Box)
    {
        FOrientedBoxData Data;
        const FVector Scale = Box->GetWorldScale();
        const FVector Extent = Box->GetBoxExtent();

        Data.Center = Box->GetWorldLocation();
        Data.Axis[0] = Box->GetForwardVector();
        Data.Axis[1] = Box->GetRightVector();
        Data.Axis[2] = Box->GetUpVector();
        Data.Extent[0] = std::fabs(Extent.X * Scale.X);
        Data.Extent[1] = std::fabs(Extent.Y * Scale.Y);
        Data.Extent[2] = std::fabs(Extent.Z * Scale.Z);

        return Data;
    }

    FVector ClosestPointOnAABB(const FVector& Point, const FAABB& Box)
    {
        return FVector(
            Clamp(Point.X, Box.Min.X, Box.Max.X),
            Clamp(Point.Y, Box.Min.Y, Box.Max.Y),
            Clamp(Point.Z, Box.Min.Z, Box.Max.Z)
        );
    }

    float PointAABBDistanceSq(const FVector& Point, const FAABB& Box)
    {
        return FVector::DistSquared(Point, ClosestPointOnAABB(Point, Box));
    }

    bool SegmentIntersectsAABB(const FVector& SegmentStart, const FVector& SegmentEnd, const FAABB& Box)
    {
        float TMin = 0.0f;
        float TMax = 1.0f;
        const FVector Direction = SegmentEnd - SegmentStart;

        for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
        {
            const float Start = SegmentStart[AxisIndex];
            const float Dir = Direction[AxisIndex];
            const float Min = Box.Min[AxisIndex];
            const float Max = Box.Max[AxisIndex];

            if (std::fabs(Dir) <= 0.0001f)
            {
                if (Start < Min || Start > Max)
                {
                    return false;
                }

                continue;
            }

            const float InvDir = 1.0f / Dir;
            float T1 = (Min - Start) * InvDir;
            float T2 = (Max - Start) * InvDir;

            if (T1 > T2)
            {
                std::swap(T1, T2);
            }

            TMin = std::max(TMin, T1);
            TMax = std::min(TMax, T2);

            if (TMin > TMax)
            {
                return false;
            }
        }

        return true;
    }

    FVector WorldToOrientedBoxSpace(const FVector& Point, const FOrientedBoxData& Box)
    {
        const FVector Delta = Point - Box.Center;
        return FVector(
            Delta.DotProduct(Box.Axis[0]),
            Delta.DotProduct(Box.Axis[1]),
            Delta.DotProduct(Box.Axis[2])
        );
    }

    FVector OrientedBoxSpaceToWorld(const FVector& Point, const FOrientedBoxData& Box)
    {
        return Box.Center +
            Box.Axis[0] * Point.X +
            Box.Axis[1] * Point.Y +
            Box.Axis[2] * Point.Z;
    }

    FVector ClosestPointOnOrientedBox(const FVector& Point, const FOrientedBoxData& Box)
    {
        const FVector LocalPoint = WorldToOrientedBoxSpace(Point, Box);
        const FVector LocalClosest(
            Clamp(LocalPoint.X, -Box.Extent[0], Box.Extent[0]),
            Clamp(LocalPoint.Y, -Box.Extent[1], Box.Extent[1]),
            Clamp(LocalPoint.Z, -Box.Extent[2], Box.Extent[2])
        );

        return OrientedBoxSpaceToWorld(LocalClosest, Box);
    }

    FVector ClosestPointOnSegment(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd)
    {
        const FVector Segment = SegmentEnd - SegmentStart;
        const float SegmentLengthSq = Segment.SizeSquared();
        if (SegmentLengthSq <= 0.0001f)
        {
            return SegmentStart;
        }

        const float T = Clamp((Point - SegmentStart).DotProduct(Segment) / SegmentLengthSq, 0.0f, 1.0f);
        return SegmentStart + Segment * T;
    }

    float SegmentSegmentDistanceSq(
        const FVector& P1,
        const FVector& Q1,
        const FVector& P2,
        const FVector& Q2)
    {
        const FVector D1 = Q1 - P1;
        const FVector D2 = Q2 - P2;
        const FVector R = P1 - P2;
        const float A = D1.DotProduct(D1);
        const float E = D2.DotProduct(D2);
        const float F = D2.DotProduct(R);

        float S = 0.0f;
        float T = 0.0f;

        if (A <= 0.0001f && E <= 0.0001f)
        {
            return FVector::DistSquared(P1, P2);
        }

        if (A <= 0.0001f)
        {
            T = Clamp(F / E, 0.0f, 1.0f);
        }
        else
        {
            const float C = D1.DotProduct(R);
            if (E <= 0.0001f)
            {
                S = Clamp(-C / A, 0.0f, 1.0f);
            }
            else
            {
                const float B = D1.DotProduct(D2);
                const float Denom = A * E - B * B;

                if (Denom != 0.0f)
                {
                    S = Clamp((B * F - C * E) / Denom, 0.0f, 1.0f);
                }

                T = (B * S + F) / E;

                if (T < 0.0f)
                {
                    T = 0.0f;
                    S = Clamp(-C / A, 0.0f, 1.0f);
                }
                else if (T > 1.0f)
                {
                    T = 1.0f;
                    S = Clamp((B - C) / A, 0.0f, 1.0f);
                }
            }
        }

        const FVector Closest1 = P1 + D1 * S;
        const FVector Closest2 = P2 + D2 * T;
        return FVector::DistSquared(Closest1, Closest2);
    }

    FCapsuleData BuildCapsuleData(const UCapsuleComponent* Capsule)
    {
        FCapsuleData Data;
        const FVector WorldScale = Capsule->GetWorldScale();

        Data.Radius = Capsule->GetCapsuleRadius() * CapsuleRadiusScale(WorldScale);
        Data.HalfHeight = Capsule->GetCapsuleHalfHeight() * CapsuleHeightScale(WorldScale);
        Data.HalfHeight = std::max(Data.HalfHeight, Data.Radius);
        Data.SegmentHalfHeight = std::max(0.0f, Data.HalfHeight - Data.Radius);
        Data.Up = Capsule->GetUpVector();

        const FVector Center = Capsule->GetWorldLocation();
        Data.SegmentStart = Center - Data.Up * Data.SegmentHalfHeight;
        Data.SegmentEnd = Center + Data.Up * Data.SegmentHalfHeight;

        return Data;
    }

    float SegmentAABBDistanceSq(const FVector& SegmentStart, const FVector& SegmentEnd, const FAABB& Box)
    {
        if (SegmentIntersectsAABB(SegmentStart, SegmentEnd, Box))
        {
            return 0.0f;
        }

        float MinDistanceSq = std::min(
            PointAABBDistanceSq(SegmentStart, Box),
            PointAABBDistanceSq(SegmentEnd, Box));

        const FVector V[8] =
        {
            FVector(Box.Min.X, Box.Min.Y, Box.Min.Z),
            FVector(Box.Max.X, Box.Min.Y, Box.Min.Z),
            FVector(Box.Min.X, Box.Max.Y, Box.Min.Z),
            FVector(Box.Max.X, Box.Max.Y, Box.Min.Z),
            FVector(Box.Min.X, Box.Min.Y, Box.Max.Z),
            FVector(Box.Max.X, Box.Min.Y, Box.Max.Z),
            FVector(Box.Min.X, Box.Max.Y, Box.Max.Z),
            FVector(Box.Max.X, Box.Max.Y, Box.Max.Z),
        };

        const int32 Edges[12][2] =
        {
            {0, 1}, {1, 3}, {3, 2}, {2, 0},
            {4, 5}, {5, 7}, {7, 6}, {6, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7},
        };

        for (const auto& Edge : Edges)
        {
            MinDistanceSq = std::min(
                MinDistanceSq,
                SegmentSegmentDistanceSq(SegmentStart, SegmentEnd, V[Edge[0]], V[Edge[1]]));
        }

        return MinDistanceSq;
    }

    float SegmentOrientedBoxDistanceSq(
        const FVector& SegmentStart,
        const FVector& SegmentEnd,
        const FOrientedBoxData& Box)
    {
        const FVector LocalStart = WorldToOrientedBoxSpace(SegmentStart, Box);
        const FVector LocalEnd = WorldToOrientedBoxSpace(SegmentEnd, Box);
        const FAABB LocalBox(
            FVector(-Box.Extent[0], -Box.Extent[1], -Box.Extent[2]),
            FVector(Box.Extent[0], Box.Extent[1], Box.Extent[2])
        );

        return SegmentAABBDistanceSq(LocalStart, LocalEnd, LocalBox);
    }

    bool OrientedBoxOrientedBoxOverlap(const FOrientedBoxData& A, const FOrientedBoxData& B)
    {
        constexpr float Epsilon = 1e-4f;

        float R[3][3];
        float AbsR[3][3];

        for (int32 i = 0; i < 3; ++i)
        {
            for (int32 j = 0; j < 3; ++j)
            {
                R[i][j] = A.Axis[i].DotProduct(B.Axis[j]); // B의 축이 A의 축 기준으로 얼마나 기울어져 있는지
                AbsR[i][j] = std::fabs(R[i][j]) + Epsilon; // 절댓값 저장. (+Epsilon은 부동소수점 오차 보정)
            }
        }

        const FVector CenterDelta = B.Center - A.Center;
        const float T[3] =
        {
            CenterDelta.DotProduct(A.Axis[0]), // B의 중심이 A.X축 방향으로 얼마나 떨어져 있는가
            CenterDelta.DotProduct(A.Axis[1]),
            CenterDelta.DotProduct(A.Axis[2])
        };

        float RA = 0.0f;
        float RB = 0.0f;

        // A의 축 3개 검사 (basis test)
        for (int32 i = 0; i < 3; ++i)
        {
            RA = A.Extent[i];
            RB = B.Extent[0] * AbsR[i][0] + B.Extent[1] * AbsR[i][1] + B.Extent[2] * AbsR[i][2];
            if (std::fabs(T[i]) > RA + RB) // 두 중심 사이 거리 > A의 투영 반지름 + B의 투영 반지름 -> 투영 구간 떨어짐 -> 충돌 X
            {
                return false;
            }
        }
        // B의 축 3개 검사 (basis test)
        for (int32 j = 0; j < 3; ++j)
        {
            RA = A.Extent[0] * AbsR[0][j] + A.Extent[1] * AbsR[1][j] + A.Extent[2] * AbsR[2][j];
            RB = B.Extent[j];
            if (std::fabs(T[0] * R[0][j] + T[1] * R[1][j] + T[2] * R[2][j]) > RA + RB)
            {
                return false;
            }
        }
        // A축 x B축 (edge test)
        for (int32 i = 0; i < 3; ++i)
        {
            for (int32 j = 0; j < 3; ++j)
            {
                RA = A.Extent[(i + 1) % 3] * AbsR[(i + 2) % 3][j] +
                     A.Extent[(i + 2) % 3] * AbsR[(i + 1) % 3][j];
                RB = B.Extent[(j + 1) % 3] * AbsR[i][(j + 2) % 3] +
                     B.Extent[(j + 2) % 3] * AbsR[i][(j + 1) % 3];

                const float Distance = std::fabs(
                    T[(i + 2) % 3] * R[(i + 1) % 3][j] -
                    T[(i + 1) % 3] * R[(i + 2) % 3][j]
                );

                if (Distance > RA + RB)
                {
                    return false;
                }
            }
        }

        return true;
    }

    FVector AABBIntersectionCenter(const FAABB& A, const FAABB& B)
    {
        FVector Min(
            std::max(A.Min.X, B.Min.X),
            std::max(A.Min.Y, B.Min.Y),
            std::max(A.Min.Z, B.Min.Z)
        );

        FVector Max(
            std::min(A.Max.X, B.Max.X),
            std::min(A.Max.Y, B.Max.Y),
            std::min(A.Max.Z, B.Max.Z)
        );

        return (Min + Max) * 0.5f;
    }

    bool BuildAABBIntersection(const FAABB& A, const FAABB& B, FAABB& OutIntersection)
    {
        if (!AABBIntersects(A, B))
        {
            return false;
        }

        OutIntersection.Min = FVector(
            std::max(A.Min.X, B.Min.X),
            std::max(A.Min.Y, B.Min.Y),
            std::max(A.Min.Z, B.Min.Z)
        );

        OutIntersection.Max = FVector(
            std::min(A.Max.X, B.Max.X),
            std::min(A.Max.Y, B.Max.Y),
            std::min(A.Max.Z, B.Max.Z)
        );

        return OutIntersection.IsValid();
    }
}

void FCollisionSystem::Tick(UWorld* World, float DeltaTime)
{
    (void)DeltaTime;

    DebugContacts.clear();

    TArray<UShapeComponent*> Shapes;
    CollectShapeComponents(World, Shapes);

    std::unordered_set<FCollisionPair, FCollisionPairHash> CurrentOverlaps;

    for (int32 i = 0; i < static_cast<int32>(Shapes.size()); ++i)
    {
        for (int32 j = i + 1; j < static_cast<int32>(Shapes.size()); ++j)
        {
            UShapeComponent* A = Shapes[i];
            UShapeComponent* B = Shapes[j];

            if (!ShouldTestPair(A, B) || !AreOverlapping(A, B))
            {
                continue;
            }

            AddDebugContact(A, B);

            FCollisionPair Pair(A, B);
            CurrentOverlaps.insert(Pair);

            if (PreviousOverlaps.find(Pair) == PreviousOverlaps.end())
            {
                HandleBeginOverlap(A, B);

                if (A->GetBlockComponent() && B->GetBlockComponent())
                {
                    HandleHit(A, B);
                }
            }

            if (A->GetBlockComponent() && B->GetBlockComponent())
            {
                ResolveBlockingOverlap(A, B);
            }
        }
    }

    for (const FCollisionPair& Pair : PreviousOverlaps)
    {
        if (CurrentOverlaps.find(Pair) == CurrentOverlaps.end())
        {
            HandleEndOverlap(Pair.A, Pair.B);
        }
    }

    PreviousOverlaps = std::move(CurrentOverlaps);
}

void FCollisionSystem::CollectShapeComponents(UWorld* World, TArray<UShapeComponent*>& OutShapes)
{
    OutShapes.clear();

    if (!World)
    {
        return;
    }

    for (AActor* Actor : World->GetActors())
    {
        if (!Actor || !Actor->IsActive())
        {
            continue;
        }

        for (UActorComponent* Component : Actor->GetComponents())
        {
            UShapeComponent* Shape = Cast<UShapeComponent>(Component);
            if (!Shape || !Shape->IsActive())
            {
                continue;
            }

            OutShapes.push_back(Shape);
        }
    }
}

bool FCollisionSystem::ShouldTestPair(const UShapeComponent* A, const UShapeComponent* B) const
{
    if (!A || !B || A == B)
    {
        return false;
    }

    if (A->GetOwner() == B->GetOwner())
    {
        return false;
    }

    return A->GetGenerateOverlapEvents() || B->GetGenerateOverlapEvents() ||
           A->GetBlockComponent() || B->GetBlockComponent();
}

bool FCollisionSystem::AreOverlapping(UShapeComponent* A, UShapeComponent* B) const
{
    if (!AABBOverlap(A, B))
    {
        return false;
    }

    if (Cast<USphereComponent>(A) && Cast<USphereComponent>(B))
    {
        return SphereSphere(A, B);
    }

    if (Cast<USphereComponent>(A) && Cast<UBoxComponent>(B))
    {
        return SphereBox(A, B);
    }

    if (Cast<UBoxComponent>(A) && Cast<USphereComponent>(B))
    {
        return SphereBox(B, A);
    }

    if (Cast<UBoxComponent>(A) && Cast<UBoxComponent>(B))
    {
        return BoxBox(A, B);
    }

    if (Cast<UCapsuleComponent>(A) && Cast<UCapsuleComponent>(B))
    {
        return CapsuleCapsule(A, B);
    }

    if (Cast<USphereComponent>(A) && Cast<UCapsuleComponent>(B))
    {
        return SphereCapsule(A, B);
    }

    if (Cast<UCapsuleComponent>(A) && Cast<USphereComponent>(B))
    {
        return SphereCapsule(B, A);
    }

    if (Cast<UBoxComponent>(A) && Cast<UCapsuleComponent>(B))
    {
        return BoxCapsule(A, B);
    }

    if (Cast<UCapsuleComponent>(A) && Cast<UBoxComponent>(B))
    {
        return BoxCapsule(B, A);
    }

    return false;
}

bool FCollisionSystem::AABBOverlap(UShapeComponent* A, UShapeComponent* B) const
{
    return AABBIntersects(A->GetWorldAABB(), B->GetWorldAABB());
}

bool FCollisionSystem::SphereSphere(UShapeComponent* A, UShapeComponent* B) const
{
    USphereComponent* SphereA = Cast<USphereComponent>(A);
    USphereComponent* SphereB = Cast<USphereComponent>(B);

    const float RadiusA = SphereA->GetSphereRadius() * MaxAbs3(SphereA->GetWorldScale());
    const float RadiusB = SphereB->GetSphereRadius() * MaxAbs3(SphereB->GetWorldScale());

    const float RadiusSum = RadiusA + RadiusB;
    const float DistSq = FVector::DistSquared(SphereA->GetWorldLocation(), SphereB->GetWorldLocation());

    return DistSq <= RadiusSum * RadiusSum;
}

bool FCollisionSystem::SphereBox(UShapeComponent* Sphere, UShapeComponent* Box) const
{
    USphereComponent* SphereComp = Cast<USphereComponent>(Sphere);
    UBoxComponent* BoxComp = Cast<UBoxComponent>(Box);

    if (!SphereComp || !BoxComp)
    {
        return false;
    }

    const FVector SphereCenter = SphereComp->GetWorldLocation();
    const float SphereRadius = SphereComp->GetSphereRadius() * MaxAbs3(SphereComp->GetWorldScale());

    const FVector Closest = ClosestPointOnOrientedBox(SphereCenter, BuildOrientedBoxData(BoxComp));
    const float DistSq = FVector::DistSquared(SphereCenter, Closest);

    return DistSq <= SphereRadius * SphereRadius;
}

bool FCollisionSystem::BoxBox(UShapeComponent* A, UShapeComponent* B) const
{
    UBoxComponent* BoxA = Cast<UBoxComponent>(A);
    UBoxComponent* BoxB = Cast<UBoxComponent>(B);
    if (!BoxA || !BoxB)
    {
        return false;
    }

    return OrientedBoxOrientedBoxOverlap(BuildOrientedBoxData(BoxA), BuildOrientedBoxData(BoxB));
}

bool FCollisionSystem::CapsuleCapsule(UShapeComponent* A, UShapeComponent* B) const
{
    UCapsuleComponent* CapsuleA = Cast<UCapsuleComponent>(A);
    UCapsuleComponent* CapsuleB = Cast<UCapsuleComponent>(B);
    if (!CapsuleA || !CapsuleB)
    {
        return false;
    }

    const FCapsuleData DataA = BuildCapsuleData(CapsuleA);
    const FCapsuleData DataB = BuildCapsuleData(CapsuleB);

    const float RadiusSum = DataA.Radius + DataB.Radius;
    return SegmentSegmentDistanceSq(
        DataA.SegmentStart,
        DataA.SegmentEnd,
        DataB.SegmentStart,
        DataB.SegmentEnd) <= RadiusSum * RadiusSum;
}

bool FCollisionSystem::SphereCapsule(UShapeComponent* Sphere, UShapeComponent* Capsule) const
{
    USphereComponent* SphereComp = Cast<USphereComponent>(Sphere);
    UCapsuleComponent* CapsuleComp = Cast<UCapsuleComponent>(Capsule);
    if (!SphereComp || !CapsuleComp)
    {
        return false;
    }

    const FCapsuleData CapsuleData = BuildCapsuleData(CapsuleComp);

    const float SphereRadius = SphereComp->GetSphereRadius() * MaxAbs3(SphereComp->GetWorldScale());
    const FVector Closest = ClosestPointOnSegment(
        SphereComp->GetWorldLocation(),
        CapsuleData.SegmentStart,
        CapsuleData.SegmentEnd);
    const float RadiusSum = SphereRadius + CapsuleData.Radius;

    return FVector::DistSquared(SphereComp->GetWorldLocation(), Closest) <= RadiusSum * RadiusSum;
}

bool FCollisionSystem::BoxCapsule(UShapeComponent* Box, UShapeComponent* Capsule) const
{
    UBoxComponent* BoxComp = Cast<UBoxComponent>(Box);
    UCapsuleComponent* CapsuleComp = Cast<UCapsuleComponent>(Capsule);
    if (!BoxComp || !CapsuleComp)
    {
        return false;
    }

    const FCapsuleData CapsuleData = BuildCapsuleData(CapsuleComp);

    return SegmentOrientedBoxDistanceSq(
        CapsuleData.SegmentStart,
        CapsuleData.SegmentEnd,
        BuildOrientedBoxData(BoxComp)) <= CapsuleData.Radius * CapsuleData.Radius;
}

void FCollisionSystem::HandleBeginOverlap(UShapeComponent* A, UShapeComponent* B)
{
    if (!A || !B)
    {
        return;
    }

    A->AddOverlap(B);
    B->AddOverlap(A);

    if (A->GetGenerateOverlapEvents())
    {
        const FCollisionEvent Event = MakeCollisionEvent(A, B, false);
        LogCollisionEvent("BeginOverlap", Event);
        A->OnHit.Broadcast(Event);

        //A->DispatchBeginOverlap(Event);
    }

    if (B->GetGenerateOverlapEvents())
    {
        const FCollisionEvent Event = MakeCollisionEvent(B, A, false);
        LogCollisionEvent("BeginOverlap", Event);
        B->DispatchBeginOverlap(Event);
    }
}

void FCollisionSystem::HandleEndOverlap(UShapeComponent* A, UShapeComponent* B)
{
    if (!A || !B)
    {
        return;
    }

    A->RemoveOverlap(B);
    B->RemoveOverlap(A);

    if (A->GetGenerateOverlapEvents())
    {
        const FCollisionEvent Event = MakeCollisionEvent(A, B, false);
        LogCollisionEvent("EndOverlap", Event);
        A->DispatchEndOverlap(Event);
    }

    if (B->GetGenerateOverlapEvents())
    {
        const FCollisionEvent Event = MakeCollisionEvent(B, A, false);
        LogCollisionEvent("EndOverlap", Event);
        B->DispatchEndOverlap(Event);
    }
}

void FCollisionSystem::HandleHit(UShapeComponent* A, UShapeComponent* B)
{
    if (!A || !B)
    {
        return;
    }

    if (A->GetBlockComponent())
    {
        const FCollisionEvent Event = MakeCollisionEvent(A, B, true);
        LogCollisionEvent("Hit", Event);
        A->DispatchHit(Event);
    }

    if (B->GetBlockComponent())
    {
        const FCollisionEvent Event = MakeCollisionEvent(B, A, true);
        LogCollisionEvent("Hit", Event);
        B->DispatchHit(Event);
    }
}

void FCollisionSystem::ResolveBlockingOverlap(UShapeComponent* A, UShapeComponent* B)
{
    if (!A || !B)
    {
        return;
    }

    AActor* ActorA = A->GetOwner();
    AActor* ActorB = B->GetOwner();
    if (!ActorA || !ActorB)
    {
        return;
    }

    const bool bAMovable = HasMovementComponent(ActorA);
    const bool bBMovable = HasMovementComponent(ActorB);
    if (!bAMovable && !bBMovable)
    {
        return;
    }

    const FVector PushA = ComputeAABBDepenetration(A->GetWorldAABB(), B->GetWorldAABB());
    if (PushA.IsNearlyZero())
    {
        return;
    }

    if (bAMovable && bBMovable)
    {
        ActorA->AddActorWorldOffset(PushA * 0.5f);
        ActorB->AddActorWorldOffset(PushA * -0.5f);
        return;
    }

    if (bAMovable)
    {
        ActorA->AddActorWorldOffset(PushA);
        return;
    }

    ActorB->AddActorWorldOffset(PushA * -1.0f);
}

void FCollisionSystem::AddDebugContact(UShapeComponent* A, UShapeComponent* B)
{
    if (!A || !B)
    {
        return;
    }

    FCollisionDebugContact Contact;
    Contact.A = A;
    Contact.B = B;
    Contact.Location = ComputeDebugContactLocation(A, B);
    Contact.bHasOverlapBounds = ComputeDebugOverlapBounds(A, B, Contact.OverlapBounds);

    FVector Delta = B->GetWorldLocation() - A->GetWorldLocation();
    Contact.Normal = Delta.GetSafeNormal();

    DebugContacts.push_back(Contact);
}

FVector FCollisionSystem::ComputeDebugContactLocation(UShapeComponent* A, UShapeComponent* B) const
{
    if (USphereComponent* SphereA = Cast<USphereComponent>(A))
    {
        if (USphereComponent* SphereB = Cast<USphereComponent>(B))
        {
            const FVector CenterA = SphereA->GetWorldLocation();
            const FVector CenterB = SphereB->GetWorldLocation();
            FVector Dir = (CenterB - CenterA).GetSafeNormal();

            if (Dir.SizeSquared() <= 0.0001f)
            {
                return (CenterA + CenterB) * 0.5f;
            }

            const float RadiusA = SphereA->GetSphereRadius() * MaxAbs3(SphereA->GetWorldScale());
            const float RadiusB = SphereB->GetSphereRadius() * MaxAbs3(SphereB->GetWorldScale());

            const FVector PointA = CenterA + Dir * RadiusA;
            const FVector PointB = CenterB - Dir * RadiusB;
            return (PointA + PointB) * 0.5f;
        }

        if (UBoxComponent* BoxB = Cast<UBoxComponent>(B))
        {
            return ClosestPointOnOrientedBox(SphereA->GetWorldLocation(), BuildOrientedBoxData(BoxB));
        }
    }

    if (UBoxComponent* BoxA = Cast<UBoxComponent>(A))
    {
        if (USphereComponent* SphereB = Cast<USphereComponent>(B))
        {
            return ClosestPointOnOrientedBox(SphereB->GetWorldLocation(), BuildOrientedBoxData(BoxA));
        }
    }

    return AABBIntersectionCenter(A->GetWorldAABB(), B->GetWorldAABB());
}

bool FCollisionSystem::ComputeDebugOverlapBounds(UShapeComponent* A, UShapeComponent* B, FAABB& OutBounds) const
{
    if (!A || !B)
    {
        return false;
    }

    return BuildAABBIntersection(A->GetWorldAABB(), B->GetWorldAABB(), OutBounds);
}

FCollisionEvent FCollisionSystem::MakeCollisionEvent(UShapeComponent* Self, UShapeComponent* Other, bool bBlockingHit) const
{
    FCollisionEvent Event;
    Event.SelfComponent = Self;
    Event.SelfActor = Self ? Self->GetOwner() : nullptr;
    Event.OtherComponent = Other;
    Event.OtherActor = Other ? Other->GetOwner() : nullptr;
    Event.bBlockingHit = bBlockingHit;

    if (const UObjectTypeComponent* SelfObjectType = FindObjectTypeComponent(Event.SelfActor))
    {
        Event.SelfObjectType = SelfObjectType->GetObjectType();
        Event.SelfGameplayTags = SelfObjectType->GetGameplayTagMask();
        Event.SelfScriptPath = FindScriptPath(Event.SelfObjectType);
    }

    if (const UObjectTypeComponent* OtherObjectType = FindObjectTypeComponent(Event.OtherActor))
    {
        Event.OtherObjectType = OtherObjectType->GetObjectType();
        Event.OtherGameplayTags = OtherObjectType->GetGameplayTagMask();
        Event.OtherScriptPath = FindScriptPath(Event.OtherObjectType);
    }

    if (!Self || !Other)
    {
        return Event;
    }

    Event.Location = ComputeDebugContactLocation(Self, Other);
    Event.Normal = (Other->GetWorldLocation() - Self->GetWorldLocation()).GetSafeNormal();
    Event.bHasOverlapBounds = ComputeDebugOverlapBounds(Self, Other, Event.OverlapBounds);

    return Event;
}

void FCollisionSystem::LogCollisionEvent(const char* EventName, const FCollisionEvent& Event) const
{
    const FString SelfActorName = Event.SelfActor ? Event.SelfActor->GetName() : FString("None");
    const FString OtherActorName = Event.OtherActor ? Event.OtherActor->GetName() : FString("None");

    UE_LOG(
        "[Collision] %s | Self=%s(%s tags=0x%X script=%s) Other=%s(%s tags=0x%X script=%s) Location=(%.2f, %.2f, %.2f) Normal=(%.2f, %.2f, %.2f) Blocking=%s",
        EventName ? EventName : "Unknown",
        SelfActorName.c_str(),
        ToString(Event.SelfObjectType),
        Event.SelfGameplayTags,
        Event.SelfScriptPath ? Event.SelfScriptPath : "",
        OtherActorName.c_str(),
        ToString(Event.OtherObjectType),
        Event.OtherGameplayTags,
        Event.OtherScriptPath ? Event.OtherScriptPath : "",
        Event.Location.X,
        Event.Location.Y,
        Event.Location.Z,
        Event.Normal.X,
        Event.Normal.Y,
        Event.Normal.Z,
        Event.bBlockingHit ? "true" : "false");
}

