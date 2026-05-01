#include "ObjectTypeComponent.h"

#include "StaticMeshComponent.h"
#include "Core/ResourceManager.h"
#include "GameFramework/AActor.h"
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
        "Boss",
        "Block",
        "Box",
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
}

void UObjectTypeComponent::Serialize(FArchive& Ar)
{
    UActorComponent::Serialize(Ar);
    
    Ar << "ObjectType" << ObjectTypeValue;
    Ar << "GameplayTags" << GameplayTagMask;
    Ar << "Auto Apply Default Tags" << bAutoApplyDefaultTags;

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

        ApplyDefaultTagsForObjectType();
    }
}

void UObjectTypeComponent::ApplyDefaultsForObjectType()
{
    ApplyDefaultTagsForObjectType();
    
    if (!bAutoApplyDefaultTags)
    {
        return;
    }
    
    ApplyDefaultMeshForObjectType();
}

void UObjectTypeComponent::ApplyDefaultTagsForObjectType()
{
    if (!bAutoApplyDefaultTags)
    {
        return;
    }

    const FObjectTypeBinding* Binding = FindObjectTypeBinding(GetObjectType());
    GameplayTagMask = GetDefaultTagsForObjectType(GetObjectType());
}

void UObjectTypeComponent::ApplyDefaultMeshForObjectType()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }
    
    const FObjectTypeBinding* Binding = FindObjectTypeBinding(GetObjectType());
    if (!Binding || !Binding->MeshPath || !Binding->MeshPath[0] == '\0')
    {
        return;
    }
    
    UStaticMeshComponent* StaticMeshComponent = FindComponentByType<UStaticMeshComponent>();
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

/*void UObjectTypeComponent::ApplyDefaultLuaForObjectType()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    const FObjectTypeBinding* Binding = FindObjectTypeBinding(GetObjectType());
    if (!Binding || !Binding->LuaPath || Binding->LuaPath[0] == '\0')
    {
        return;
    }

    ULuaScriptComponent* LuaComponent = FindComponentByType<ULuaScriptComponent>(OwnerActor);
    if (!LuaComponent)
    {
        LuaComponent = OwnerActor->AddComponent<ULuaScriptComponent>();
    }

    LuaComponent->SetScriptPath(Binding->LuaPath);
}*/

