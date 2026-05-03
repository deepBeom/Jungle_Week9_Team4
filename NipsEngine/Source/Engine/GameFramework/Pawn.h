#pragma once

#include "GameFramework/Actor.h"

class UCameraComponent;
class UStaticMeshComponent;

class APawn : public AActor
{
public:
    DECLARE_CLASS(APawn, AActor)
    APawn() = default;

    void InitDefaultComponents() override;
    UCameraComponent* GetCameraComponent() const;

    FVector GetForwardVector() const;
    FVector GetRightVector() const;
    FVector GetUpVector() const;
};
