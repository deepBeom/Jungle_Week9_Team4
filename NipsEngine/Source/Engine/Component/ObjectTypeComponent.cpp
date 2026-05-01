#include "ObjectTypeComponent.h"

#include <algorithm>
#include <cstring>

#include "Asset/StaticMesh.h"
#include "Component/ShapeComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Core/GameObjectTypes.h"
#include "Core/ResourceManager.h"
#include "GameFramework/Actor.h"
#include "Object/ObjectFactory.h"

DEFINE_CLASS(UObjectTypeComponent, UActorComponent)
REGISTER_FACTORY(UObjectTypeComponent)

namespace
{
    const char* ObjectTypeNames[] =
    {
        "None",
        "Player",
        "Coin",
        "Block",
        "Box",
        "Boss",
    };

    template<typename T>
    T* FindComponentByType(AActor* Actor)
    {
        if (!Actor)
        {
            return nullptr;
        }

        for (UActorComponent* Component : Actor->GetComponents())
        {
            if (T* TypedComponent = Cast<T>(Component))
            {
                return TypedComponent;
            }
        }

        return nullptr;
    }
}

uint32 GetDefaultTagsForObjectType(EObjectType Type)
{
    switch (Type)
    {
    case EObjectType::Player:
        return GT_Player;

    case EObjectType::Coin:
        return GT_Collectible | GT_TriggerOnly;

    case EObjectType::Boss:
        return GT_Enemy | GT_Boss | GT_Damageable;

    case EObjectType::Block:
        return GT_Solid | GT_Environment;

    case EObjectType::Box:
        return GT_Solid | GT_Environment;

    case EObjectType::None:
    default:
        return GT_None;
    }
}

void UObjectTypeComponent::Serialize(FArchive& Ar)
{
    UActorComponent::Serialize(Ar);

    Ar << "ObjectType" << ObjectTypeValue;
    Ar << "GameplayTags" << GameplayTagMask;
    Ar << "Auto Apply Default Tags" << bAutoApplyDefaultTags;
    Ar << "Auto Apply Object Defaults" << bAutoApplyObjectDefaults;

    if (Ar.IsLoading())
    {
        if (ObjectTypeValue < 0 || ObjectTypeValue >= static_cast<int32>(EObjectType::MAX))
        {
            ObjectTypeValue = static_cast<int32>(EObjectType::None);
        }
    }
}

void UObjectTypeComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
    UActorComponent::GetEditableProperties(OutProps);

    FPropertyDescriptor ObjectTypeProp;
    ObjectTypeProp.Name = "ObjectType";
    ObjectTypeProp.Type = EPropertyType::Enum;
    ObjectTypeProp.ValuePtr = &ObjectTypeValue;
    ObjectTypeProp.EnumNames = ObjectTypeNames;
    ObjectTypeProp.EnumCount = static_cast<uint32>(EObjectType::MAX);
    OutProps.push_back(ObjectTypeProp);

    OutProps.push_back({ "GameplayTags", EPropertyType::Int, &GameplayTagMask });
    OutProps.push_back({ "Auto Apply Default Tags", EPropertyType::Bool, &bAutoApplyDefaultTags });
    OutProps.push_back({ "Auto Apply Object Defaults", EPropertyType::Bool, &bAutoApplyObjectDefaults });
}

void UObjectTypeComponent::PostEditProperty(const char* PropertyName)
{
    UActorComponent::PostEditProperty(PropertyName);

    if (std::strcmp(PropertyName, "ObjectType") == 0)
    {
        if (ObjectTypeValue < 0 || ObjectTypeValue >= static_cast<int32>(EObjectType::MAX))
        {
            ObjectTypeValue = static_cast<int32>(EObjectType::None);
        }

        ApplyDefaultsForObjectType();
    }
}

void UObjectTypeComponent::ApplyDefaultsForObjectType()
{
    ApplyDefaultTagsForObjectType();

    if (!bAutoApplyObjectDefaults)
    {
        return;
    }

    ApplyDefaultMeshForObjectType();
    ApplyDefaultCollisionForObjectType();
}

void UObjectTypeComponent::ApplyDefaultTagsForObjectType()
{
    if (!bAutoApplyDefaultTags)
    {
        return;
    }

    if (const FObjectTypeBinding* Binding = FindObjectTypeBinding(GetObjectType()))
    {
        GameplayTagMask = Binding->DefaultTags;
    }
    else
    {
        GameplayTagMask = GetDefaultTagsForObjectType(GetObjectType());
    }
}

void UObjectTypeComponent::ApplyDefaultMeshForObjectType()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    const FObjectTypeBinding* Binding = FindObjectTypeBinding(GetObjectType());
    if (!Binding || !Binding->MeshPath || Binding->MeshPath[0] == '\0')
    {
        return;
    }

    UStaticMeshComponent* StaticMeshComponent = FindComponentByType<UStaticMeshComponent>(OwnerActor);
    if (!StaticMeshComponent)
    {
        StaticMeshComponent = OwnerActor->AddComponent<UStaticMeshComponent>();
        if (!OwnerActor->GetRootComponent())
        {
            OwnerActor->SetRootComponent(StaticMeshComponent);
        }
    }

    UStaticMesh* Mesh = FResourceManager::Get().LoadStaticMesh(Binding->MeshPath);
    StaticMeshComponent->SetStaticMesh(Mesh);
}

void UObjectTypeComponent::ApplyDefaultCollisionForObjectType()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    const FObjectTypeBinding* Binding = FindObjectTypeBinding(GetObjectType());
    if (!Binding || Binding->CollisionShape == EObjectCollisionShape::None)
    {
        return;
    }

    UShapeComponent* ShapeComponent = FindComponentByType<UShapeComponent>(OwnerActor);

    const bool bNeedsNewShape =
       (Binding->CollisionShape == EObjectCollisionShape::Sphere && !Cast<USphereComponent>(ShapeComponent)) ||
       (Binding->CollisionShape == EObjectCollisionShape::Box && !Cast<UBoxComponent>(ShapeComponent)) ||
       (Binding->CollisionShape == EObjectCollisionShape::Capsule && !Cast<UCapsuleComponent>(ShapeComponent));

    if (bNeedsNewShape && ShapeComponent)
    {
        OwnerActor->RemoveComponent(ShapeComponent);
        ShapeComponent = nullptr;
    }

    if (!ShapeComponent)
    {
        switch (Binding->CollisionShape)
        {
        case EObjectCollisionShape::Sphere:
            ShapeComponent = OwnerActor->AddComponent<USphereComponent>();
            break;

        case EObjectCollisionShape::Box:
            ShapeComponent = OwnerActor->AddComponent<UBoxComponent>();
            break;

        case EObjectCollisionShape::Capsule:
            ShapeComponent = OwnerActor->AddComponent<UCapsuleComponent>();
            break;

        case EObjectCollisionShape::None:
        default:
            break;
        }
    }

    if (!ShapeComponent)
    {
        return;
    }

    ShapeComponent->SetGenerateOverlapEvents(Binding->bGenerateOverlapEvents);
    ShapeComponent->SetBlockComponent(Binding->bBlockComponent);

    if (USceneComponent* Root = OwnerActor->GetRootComponent())
    {
        ShapeComponent->AttachToComponent(Root);
    }

    FitCollisionToStaticMesh(ShapeComponent, FindComponentByType<UStaticMeshComponent>(OwnerActor));
}

void UObjectTypeComponent::FitCollisionToStaticMesh(UShapeComponent* ShapeComponent, UStaticMeshComponent* StaticMeshComponent)
{
    if (!ShapeComponent || !StaticMeshComponent || !StaticMeshComponent->HasValidMesh())
    {
        return;
    }

    UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
    if (!StaticMesh)
    {
        return;
    }

    const FAABB& LocalBounds = StaticMesh->GetLocalBounds();
    if (!LocalBounds.IsValid())
    {
        return;
    }

    const FVector Center = LocalBounds.GetCenter();
    const FVector Extent = LocalBounds.GetExtent();
    ShapeComponent->SetRelativeLocation(Center);

    if (UBoxComponent* Box = Cast<UBoxComponent>(ShapeComponent))
    {
        Box->SetBoxExtent(Extent);
        return;
    }

    if (USphereComponent* Sphere = Cast<USphereComponent>(ShapeComponent))
    {
        // Bounding sphere: corner distance encloses the whole mesh AABB.
        Sphere->SetSphereRadius(Extent.Size());
        return;
    }

    if (UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(ShapeComponent))
    {
        float Radius = std::max(Extent.X, Extent.Y);
        float HalfHeight = Extent.Z;
        FVector CapsuleRotation = FVector::ZeroVector;

        if (Extent.X >= Extent.Y && Extent.X >= Extent.Z)
        {
            Radius = std::max(Extent.Y, Extent.Z);
            HalfHeight = Extent.X;
            CapsuleRotation = FVector(0.0f, 90.0f, 0.0f);
        }
        else if (Extent.Y >= Extent.X && Extent.Y >= Extent.Z)
        {
            Radius = std::max(Extent.X, Extent.Z);
            HalfHeight = Extent.Y;
            CapsuleRotation = FVector(-90.0f, 0.0f, 0.0f);
        }

        Capsule->SetRelativeRotation(CapsuleRotation);
        Capsule->SetCapsuleSize(Radius, HalfHeight);
    }
}
