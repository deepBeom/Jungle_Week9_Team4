#include "PIEController.h"
#include "Engine/Viewport/ViewportCamera.h"
#include "Component/PrimitiveComponent.h"
#include "Component/ShapeComponent.h"
#include "Core/ActorTags.h"
#include "Engine/Collision/CollisionSystem.h"
#include "Engine/Input/InputSystem.h"
#include "GameFramework/Actor.h"
#include "GameFramework/World.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float MousePushDistance = 2.0f;

    float MaxAbs3(const FVector& V)
    {
        return std::max({ std::fabs(V.X), std::fabs(V.Y), std::fabs(V.Z) });
    }

    float Clamp(float Value, float Min, float Max)
    {
        return std::max(Min, std::min(Value, Max));
    }

    FVector ClosestPointOnAABB(const FVector& Point, const FAABB& Box)
    {
        return FVector(
            Clamp(Point.X, Box.Min.X, Box.Max.X),
            Clamp(Point.Y, Box.Min.Y, Box.Max.Y),
            Clamp(Point.Z, Box.Min.Z, Box.Max.Z));
    }

    float PointAABBDistanceSq(const FVector& Point, const FAABB& Box)
    {
        return FVector::DistSquared(Point, ClosestPointOnAABB(Point, Box));
    }
}


void FPIEController::Tick(float InDeltaTime) {
    DeltaTime = InDeltaTime;
    if (!Camera)
        return;
    if (!bTargetLocationInitialized)
    {
        TargetLocation = Camera->GetLocation();
        bTargetLocationInitialized = true;
    }
    constexpr float LocationLerpSpeed = 12.0f;
    const FVector   CurrentLocation = Camera->GetLocation();
    const float     LerpAlpha = MathUtil::Clamp(DeltaTime * LocationLerpSpeed, 0.0f, 1.0f);
    Camera->SetLocation(CurrentLocation + (TargetLocation - CurrentLocation) * LerpAlpha);

    if (bCollectionKeyHeld)
    {
        TickCollectionArea(DeltaTime);
    }
}

void FPIEController::OnMouseMove(float DeltaX, float DeltaY)
{
    if (!Camera)
        return;

    if (Camera->IsOrthographic())
    {
        // Pan: scale movement proportionally to current ortho zoom level
        const float   PanScale = Camera->GetOrthoHeight() * 0.002f;
        const FVector Right = Camera->GetEffectiveRight();
        const FVector Up = Camera->GetEffectiveUp();
        TargetLocation += Right * (-DeltaX * PanScale) + Up * (DeltaY * PanScale);
    }
    else
    {
        // Accumulate yaw/pitch and rebuild rotation quaternion
        const float RotationSpeed = 0.15f;
        Yaw += DeltaX * RotationSpeed;
        Pitch -= DeltaY * RotationSpeed;
        Pitch = MathUtil::Clamp(Pitch, -89.f, 89.f);
        UpdateCameraRotation();
    }
}

void FPIEController::OnLeftMouseClick(float X, float Y)
{
    PushActorAtScreenPosition(X, Y);
}

void FPIEController::OnLeftMouseDragEnd(float X, float Y)
{
    // Does nothing for now
}

void FPIEController::OnLeftMouseButtonUp(float X, float Y)
{
    // Does nothing for now
}

void FPIEController::OnRightMouseClick(float DeltaX, float DeltaY)
{
    // Does nothing for now
}

void FPIEController::OnLeftMouseDrag(float X, float Y)
{
    // Does nothing for now
}

void FPIEController::OnRightMouseDrag(float DeltaX, float DeltaY)
{
    // Does nothing for now
}

void FPIEController::OnMiddleMouseDrag(float DeltaX, float DeltaY)
{
    // Does nothing for now
}

void FPIEController::OnKeyPressed(int VK)
{
    switch (VK)
    {
    case VK_ESCAPE:
        if (OnRequestEndPIE)
            OnRequestEndPIE();
        break;
    case VK_SPACE:
        bCollectionKeyHeld = true;
        CurrentCollectionRadius = CollectionMinRadius;
        break;
    }
}

void FPIEController::OnKeyDown(int VK)
{
    // WASD continuous movement + arrow key rotation (called every frame the key is held)
    if (!Camera)
        return;

    const float MoveSpeed = 10.f;
    FVector     Move = FVector(0, 0, 0);
    //switch (VK)
    //{
    //case 'W':
    //    Move.X += MoveSpeed;
    //    break;
    //case 'A':
    //    Move.Y -= MoveSpeed;
    //    break;
    //case 'S':
    //    Move.X -= MoveSpeed;
    //    break;
    //case 'D':
    //    Move.Y += MoveSpeed;
    //    break;
    //}

    //if (Move.X != 0.f || Move.Y != 0.f)
    //{
    //    // Constrain movement to the horizontal plane (no flying)
    //    Move *= DeltaTime;
    //    FVector Forward = Camera->GetForwardVector();
    //    Forward.Z = 0.f;
    //    if (Forward.Size() > 1e-4f)
    //        Forward.Normalize();
    //    FVector Right = Camera->GetRightVector();
    //    Right.Z = 0.f;
    //    if (Right.Size() > 1e-4f)
    //        Right.Normalize();
    //    TargetLocation += Forward * Move.X + Right * Move.Y;
    //}

    // Allow flying
    switch (VK)
    {
    case 'W':
        Move += Camera->GetForwardVector() * MoveSpeed;
        break;
    case 'S':
        Move += Camera->GetForwardVector() * -MoveSpeed;
        break;
    case 'D':
        Move += Camera->GetRightVector() * MoveSpeed;
        break;
    case 'A':
        Move += Camera->GetRightVector() * -MoveSpeed;
        break;
    }

    if (Move.X != 0.f || Move.Y != 0.f)
    {
        // Constrain movement to the horizontal plane (no flying)
        Move *= DeltaTime;
        TargetLocation += Move;
    }

    // Arrow key rotation
    constexpr float AngleVelocity = 60.f;
    bool            bRotationChanged = false;
    switch (VK)
    {
    case VK_LEFT:
        Yaw -= AngleVelocity * DeltaTime;
        bRotationChanged = true;
        break;
    case VK_RIGHT:
        Yaw += AngleVelocity * DeltaTime;
        bRotationChanged = true;
        break;
    case VK_UP:
        Pitch += AngleVelocity * DeltaTime;
        bRotationChanged = true;
        break;
    case VK_DOWN:
        Pitch -= AngleVelocity * DeltaTime;
        bRotationChanged = true;
        break;
    }
    if (bRotationChanged)
    {
        Pitch = MathUtil::Clamp(Pitch, -89.f, 89.f);
        UpdateCameraRotation();
    }
}


void FPIEController::OnKeyReleased(int VK)
{
    if (VK == VK_SPACE)
    {
        ReleaseCollectionArea();
    }
}

void FPIEController::OnWheelScrolled(float Notch)
{
    // Does nothing for now
}

void FPIEController::SetCamera(FViewportCamera* InCamera)
{
    if (!InCamera)
        return;
    Camera = InCamera;

    TargetLocation = Camera->GetLocation();
    bTargetLocationInitialized = true;

    // Initialize Yaw/Pitch from camera's current orientation so the first
    // mouse move does not snap the camera to a default forward direction.
    FVector Forward = Camera->GetForwardVector();
    Pitch = MathUtil::RadiansToDegrees(std::asin(MathUtil::Clamp(Forward.Z, -1.f, 1.f)));
    Yaw   = MathUtil::RadiansToDegrees(std::atan2(Forward.Y, Forward.X));
}

void FPIEController::SetCamera(FViewportCamera& InCamera) { SetCamera(&InCamera); }

void FPIEController::UpdateCameraRotation()
{
    if (!Camera)
        return;

    const float PitchRad = MathUtil::DegreesToRadians(Pitch);
    const float YawRad = MathUtil::DegreesToRadians(Yaw);

    FVector Forward(std::cos(PitchRad) * std::cos(YawRad), std::cos(PitchRad) * std::sin(YawRad), std::sin(PitchRad));
    Forward = Forward.GetSafeNormal();

    FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
        return;

    FVector Up = FVector::CrossProduct(Forward, Right).GetSafeNormal();

    FMatrix RotMat = FMatrix::Identity;
    RotMat.SetAxes(Forward, Right, Up);

    FQuat NewRotation(RotMat);
    NewRotation.Normalize();
    Camera->SetRotation(NewRotation);
}

void FPIEController::ResetTargetLocation()
{
    if (Camera)
        TargetLocation = Camera->GetLocation();
}

void FPIEController::PushActorAtScreenPosition(float X, float Y)
{
    if (!World || !Camera || ViewportWidth <= 0.0f || ViewportHeight <= 0.0f)
    {
        return;
    }

    float RayX = X;
    float RayY = Y;
    if (InputSystem::Get().IsMouseLocked())
    {
        RayX = ViewportWidth * 0.5f;
        RayY = ViewportHeight * 0.5f;
    }

    const FRay Ray = Camera->DeprojectScreenToWorld(RayX, RayY, ViewportWidth, ViewportHeight);
    const FVector End = Ray.Origin + Ray.Direction * Camera->GetFarPlane();

    FHitResult Hit;
    if (!World->GetCollisionSystem().LineTraceSingle(World, Ray.Origin, End, Hit, ActorTags::Boat, true))
    {
        return;
    }

    AActor* HitActor = Hit.HitComponent ? Hit.HitComponent->GetOwner() : nullptr;
    if (!HitActor || HitActor->CompareTag(ActorTags::Boat) || HitActor->IsPendingDestroy())
    {
        return;
    }

    FVector PushDir = FVector::ZeroVector;
    if (AActor* Boat = FindBoatActor())
    {
        PushDir = (HitActor->GetActorLocation() - Boat->GetActorLocation()).GetSafeNormal2D();
    }

    if (PushDir.IsNearlyZero())
    {
        PushDir = Ray.Direction.GetSafeNormal2D();
    }

    if (PushDir.IsNearlyZero())
    {
        PushDir = (HitActor->GetActorLocation() - Camera->GetLocation()).GetSafeNormal2D();
    }

    if (!PushDir.IsNearlyZero())
    {
        HitActor->AddActorWorldOffset(PushDir * MousePushDistance);
    }
}

void FPIEController::TickCollectionArea(float DeltaTime)
{
    AActor* Boat = FindBoatActor();
    if (!World || !Boat)
    {
        return;
    }

    CurrentCollectionRadius = Clamp(
        CurrentCollectionRadius + CollectionGrowthRate * DeltaTime,
        CollectionMinRadius,
        CollectionMaxRadius);

    World->SetCollectionDebugCircle(
        true,
        Boat->GetActorLocation(),
        CurrentCollectionRadius,
        FColor(0, 220, 255));
}

void FPIEController::ReleaseCollectionArea()
{
    bCollectionKeyHeld = false;

    AActor* Boat = FindBoatActor();
    if (World)
    {
        World->SetCollectionDebugCircle(false);
    }

    if (!World || !Boat)
    {
        return;
    }

    const FVector CollectionCenter = Boat->GetActorLocation();
    const float CollectionRadius = CurrentCollectionRadius > 0.0f
        ? CurrentCollectionRadius
        : CollectionMinRadius;

    TArray<AActor*> ActorsToCollect;
    for (AActor* Actor : World->GetActors())
    {
        if (!Actor || Actor == Boat || Actor->IsPendingDestroy() || !IsCollectibleActor(Actor))
        {
            continue;
        }

        if (IsActorInsideCollectionRadius(Actor, CollectionCenter, CollectionRadius))
        {
            ActorsToCollect.push_back(Actor);
        }
    }

    for (AActor* Actor : ActorsToCollect)
    {
        if (Actor && !Actor->IsPendingDestroy())
        {
            Actor->Destroy();
        }
    }

    CurrentCollectionRadius = CollectionMinRadius;
}

AActor* FPIEController::FindBoatActor() const
{
    if (!World)
    {
        return nullptr;
    }

    for (AActor* Actor : World->GetActors())
    {
        if (Actor && Actor->CompareTag(ActorTags::Boat) && !Actor->IsPendingDestroy())
        {
            return Actor;
        }
    }

    return nullptr;
}

bool FPIEController::IsCollectibleActor(const AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    return Actor->CompareTag(ActorTags::Trash) ||
           Actor->CompareTag(ActorTags::Resource) ||
           Actor->CompareTag(ActorTags::Recyclable) ||
           Actor->CompareTag(ActorTags::Premium);
}

bool FPIEController::IsActorInsideCollectionRadius(const AActor* Actor, const FVector& Center, float Radius) const
{
    if (!Actor || Radius <= 0.0f)
    {
        return false;
    }

    const float RadiusSq = Radius * Radius;

    for (UPrimitiveComponent* Primitive : Actor->GetPrimitiveComponents())
    {
        if (!Primitive || !Primitive->IsActive())
        {
            continue;
        }

        const FAABB& Bounds = Primitive->GetWorldAABB();
        if (Bounds.IsValid() && PointAABBDistanceSq(Center, Bounds) <= RadiusSq)
        {
            return true;
        }
    }

    return FVector::DistSquared(Center, Actor->GetActorLocation()) <= RadiusSq;
}
