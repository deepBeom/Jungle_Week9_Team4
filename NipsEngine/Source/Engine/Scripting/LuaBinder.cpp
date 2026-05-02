#include "Core/EnginePCH.h"
#include "Engine/Scripting/LuaBinder.h"

#include "Engine/Component/ActorComponent.h"
#include "Engine/Core/CollisionTypes.h"
#include "Engine/Core/Logging/Timer.h"
#include "Engine/GameFramework/Actor.h"
#include "Engine/Runtime/Engine.h"

namespace
{
    // A valid actor can still be pending destroy; call sites choose strictness.
    bool IsValidActorObject(AActor* Actor)
    {
        return Actor && UObject::IsValid(Actor);
    }

    // For gameplay callbacks/bindings, pending-destroy actors are treated as unusable.
    bool IsUsableActor(AActor* Actor)
    {
        return IsValidActorObject(Actor) && !Actor->IsPendingDestroy();
    }

    bool IsUsableComponent(UActorComponent* Component)
    {
        return Component && UObject::IsValid(Component);
    }

    std::string SafeObjectName(UObject* Object)
    {
        return (Object && UObject::IsValid(Object))
            ? static_cast<std::string>(Object->GetName())
            : std::string();
    }

    // Type-name lookup used by both GetComponent and FindComponentByClass.
    UActorComponent* FindActorComponentByType(AActor* Actor, const std::string& TypeName)
    {
        if (!IsUsableActor(Actor))
        {
            return nullptr;
        }

        const FString RequestedType = TypeName;
        const FString PrefixedType = RequestedType.rfind("U", 0) == 0
            ? RequestedType
            : ("U" + RequestedType);

        for (UActorComponent* Component : Actor->GetComponents())
        {
            if (!IsUsableComponent(Component))
            {
                continue;
            }

            const FString ComponentType = Component->GetTypeInfo()->name;
            if (ComponentType == RequestedType || ComponentType == PrefixedType)
            {
                return Component;
            }
        }

        return nullptr;
    }

    void BindMathTypes(sol::state& Lua)
    {
        Lua.new_usertype<FVector>(
            "Vec3",
            sol::constructors<FVector(), FVector(float, float, float)>(),
            "X", &FVector::X,
            "Y", &FVector::Y,
            "Z", &FVector::Z);

        Lua.new_usertype<FHitResult>(
            "HitInfo",
            "Distance", &FHitResult::Distance,
            "Location", &FHitResult::Location,
            "Normal", &FHitResult::Normal,
            "FaceIndex", &FHitResult::FaceIndex,
            "IsValid", [](const FHitResult* Hit)
            {
                return Hit && Hit->IsValid();
            });
    }

    void BindComponentType(sol::state& Lua)
    {
        Lua.new_usertype<UActorComponent>(
            "Component",
            "GetName", [](UActorComponent* Component) -> std::string
            {
                return SafeObjectName(Component);
            },
            "GetTypeName", [](UActorComponent* Component) -> std::string
            {
                return IsUsableComponent(Component)
                    ? Component->GetTypeInfo()->name
                    : std::string();
            },
            "GetOwner", [](UActorComponent* Component) -> AActor*
            {
                if (!IsUsableComponent(Component))
                {
                    return nullptr;
                }

                AActor* Owner = Component->GetOwner();
                return IsUsableActor(Owner) ? Owner : nullptr;
            },
            "SetActive", [](UActorComponent* Component, bool bEnabled)
            {
                if (IsUsableComponent(Component))
                {
                    Component->SetActive(bEnabled);
                }
            },
            "IsActive", [](UActorComponent* Component)
            {
                return IsUsableComponent(Component) && Component->IsActive();
            });
    }

    void BindActorType(sol::state& Lua)
    {
        Lua.new_usertype<AActor>(
            "Actor",
            "GetName", [](AActor* Actor) -> std::string
            {
                return SafeObjectName(Actor);
            },
            "GetPosition", [](AActor* Actor)
            {
                return IsUsableActor(Actor)
                    ? Actor->GetActorLocation()
                    : FVector::ZeroVector;
            },
            "SetPosition", [](AActor* Actor, float X, float Y, float Z)
            {
                if (IsUsableActor(Actor))
                {
                    Actor->SetActorLocation(FVector(X, Y, Z));
                }
            },
            "AddPosition", [](AActor* Actor, float X, float Y, float Z)
            {
                if (IsUsableActor(Actor))
                {
                    Actor->AddActorWorldOffset(FVector(X, Y, Z));
                }
            },
            "GetRotation", [](AActor* Actor)
            {
                return IsUsableActor(Actor)
                    ? Actor->GetActorRotation()
                    : FVector::ZeroVector;
            },
            "SetRotation", [](AActor* Actor, float X, float Y, float Z)
            {
                if (IsUsableActor(Actor))
                {
                    Actor->SetActorRotation(FVector(X, Y, Z));
                }
            },
            "GetScale", [](AActor* Actor)
            {
                return IsUsableActor(Actor)
                    ? Actor->GetActorScale()
                    : FVector(1.0f, 1.0f, 1.0f);
            },
            "SetScale", [](AActor* Actor, float X, float Y, float Z)
            {
                if (IsUsableActor(Actor))
                {
                    Actor->SetActorScale(FVector(X, Y, Z));
                }
            },
            "GetForwardVector", [](AActor* Actor)
            {
                return IsUsableActor(Actor)
                    ? Actor->GetActorForward()
                    : FVector(0.0f, 0.0f, 1.0f);
            },
            "SetActive", [](AActor* Actor, bool bEnabled)
            {
                if (IsUsableActor(Actor))
                {
                    Actor->SetActive(bEnabled);
                }
            },
            "GetTag", [](AActor* Actor) -> std::string
            {
                return IsUsableActor(Actor)
                    ? Actor->GetTag()
                    : std::string();
            },
            "SetTag", [](AActor* Actor, const std::string& Tag)
            {
                if (IsUsableActor(Actor))
                {
                    Actor->SetTag(Tag);
                }
            },
            "CompareTag", [](AActor* Actor, const std::string& Tag)
            {
                return IsUsableActor(Actor) && Actor->CompareTag(Tag);
            },
            "Destroy", [](AActor* Actor)
            {
                if (IsUsableActor(Actor))
                {
                    Actor->Destroy();
                }
            },
            "GetComponent", [](AActor* Actor, const std::string& TypeName) -> UActorComponent*
            {
                return FindActorComponentByType(Actor, TypeName);
            },
            "FindComponentByClass", [](AActor* Actor, const std::string& TypeName) -> UActorComponent*
            {
                return FindActorComponentByType(Actor, TypeName);
            },
            "IsPendingDestroy", [](AActor* Actor)
            {
                return IsValidActorObject(Actor) && Actor->IsPendingDestroy();
            },
            "IsValid", [](AActor* Actor)
            {
                return IsUsableActor(Actor);
            });
    }
}

void LuaBinder::BindEngineTypes(sol::state& Lua)
{
    // Keep registration split by domain so additions stay localized.
    BindMathTypes(Lua);
    BindComponentType(Lua);
    BindActorType(Lua);
}

void LuaBinder::BindGlobalFunctions(sol::state& Lua)
{
    Lua.set_function("Log", [](const std::string& Message)
    {
        printf("[Lua] %s\n", Message.c_str());
    });

    Lua.set_function("Warning", [](const std::string& Message)
    {
        printf("[Lua Warning] %s\n", Message.c_str());
    });

    Lua.set_function("Error", [](const std::string& Message)
    {
        printf("[Lua Error] %s\n", Message.c_str());
    });

    Lua.set_function("GetTimeSeconds", []() -> double
    {
        return (GEngine && GEngine->GetTimer()) ? GEngine->GetTimer()->GetTotalTime() : 0.0;
    });

    Lua.set_function("GetFrameCount", []() -> uint64
    {
        return GEngine ? GEngine->GetFrameCount() : 0;
    });
}
