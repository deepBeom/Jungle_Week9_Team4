#include "GameFramework/Pawn.h"

#include "Component/CameraComponent.h"
#include "Component/SceneComponent.h"
#include "Component/StaticMeshComponent.h"

DEFINE_CLASS(APawn, AActor)
REGISTER_FACTORY(APawn)

void APawn::InitDefaultComponents()
{
    ControllerScriptPath = "Asset/Scripts/PlayerController.lua";

    USceneComponent* RootComponent = AddComponent<USceneComponent>();
    SetRootComponent(RootComponent);

    USceneComponent* MovementRootComponent = AddComponent<USceneComponent>();
    MovementRootComponent->AttachToComponent(RootComponent);

    UStaticMeshComponent* MeshComponent = AddComponent<UStaticMeshComponent>();
    MeshComponent->AttachToComponent(MovementRootComponent);

    UCameraComponent* CameraComponent = AddComponent<UCameraComponent>();
    CameraComponent->AttachToComponent(RootComponent);
    CameraComponent->SetRelativeLocation(FVector(-6.0f, 0.0f, 4.0f));
    CameraComponent->LookAt(RootComponent->GetWorldLocation());
}

UCameraComponent* APawn::GetCameraComponent() const
{
    for (UActorComponent* Component : GetComponents())
    {
        if (UCameraComponent* CameraComponent = Cast<UCameraComponent>(Component))
        {
            return CameraComponent;
        }
    }

    return nullptr;
}

USceneComponent* APawn::GetMovementRootComponent() const
{
    USceneComponent* Root = GetRootComponent();
    if (!Root)
    {
        return nullptr;
    }

    for (USceneComponent* Child : Root->GetChildren())
    {
        if (Child && Child->GetTypeInfo()->name == "USceneComponent")
        {
            return Child;
        }
    }

    return Root;
}

FVector APawn::GetForwardVector() const
{
    if (USceneComponent* MovementRoot = GetMovementRootComponent())
    {
        return MovementRoot->GetForwardVector();
    }

    return FVector(1.0f, 0.0f, 0.0f);
}

FVector APawn::GetRightVector() const
{
    if (USceneComponent* MovementRoot = GetMovementRootComponent())
    {
        return MovementRoot->GetRightVector();
    }

    return FVector(0.0f, 1.0f, 0.0f);
}

FVector APawn::GetUpVector() const
{
    if (USceneComponent* MovementRoot = GetMovementRootComponent())
    {
        return MovementRoot->GetUpVector();
    }

    return FVector(0.0f, 0.0f, 1.0f);
}

