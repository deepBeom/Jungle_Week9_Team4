#include "GameFramework/Pawn.h"

#include "Component/CameraComponent.h"
#include "Component/SceneComponent.h"
#include "Component/StaticMeshComponent.h"

DEFINE_CLASS(APawn, AActor)
REGISTER_FACTORY(APawn)

void APawn::InitDefaultComponents()
{
    USceneComponent* RootComponent = AddComponent<UStaticMeshComponent>();
    SetRootComponent(RootComponent);

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

FVector APawn::GetForwardVector() const
{
    if (USceneComponent* Root = GetRootComponent())
    {
        return Root->GetForwardVector();
    }

    return FVector(1.0f, 0.0f, 0.0f);
}

FVector APawn::GetRightVector() const
{
    if (USceneComponent* Root = GetRootComponent())
    {
        return Root->GetRightVector();
    }

    return FVector(0.0f, 1.0f, 0.0f);
}

FVector APawn::GetUpVector() const
{
    if (USceneComponent* Root = GetRootComponent())
    {
        return Root->GetUpVector();
    }

    return FVector(0.0f, 0.0f, 1.0f);
}

