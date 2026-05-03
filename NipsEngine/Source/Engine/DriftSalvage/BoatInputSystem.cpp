#include "DriftSalvage/BoatInputSystem.h"

#include <Windows.h>

#include "Core/ActorTags.h"
#include "Engine/Input/InputSystem.h"
#include "GameFramework/Actor.h"
#include "GameFramework/World.h"

namespace
{
AActor* FindBoat(UWorld* World)
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
}

void FBoatInputSystem::Tick(UWorld* World, float DeltaTime)
{
    if (!World || DeltaTime <= 0.0f)
    {
        return;
    }

    AActor* Boat = FindBoat(World);
    if (!Boat)
    {
        return;
    }

    InputSystem& Input = InputSystem::Get();

    FVector Delta = FVector::ZeroVector;
    if (Input.GetKey(VK_UP))    { Delta.X += 1.0f; }
    if (Input.GetKey(VK_DOWN))  { Delta.X -= 1.0f; }
    if (Input.GetKey(VK_RIGHT)) { Delta.Y += 1.0f; }
    if (Input.GetKey(VK_LEFT))  { Delta.Y -= 1.0f; }

    if (Delta.IsNearlyZero())
    {
        return;
    }

    // 대각선 이동 시 속도 보정.
    Delta = Delta.GetSafeNormal2D() * (MoveSpeed * DeltaTime);
    Boat->AddActorWorldOffset(Delta);
}