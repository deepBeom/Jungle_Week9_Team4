#include "Core/EnginePCH.h"
#include "Engine/Scripting/LuaBinder.h"

#include "Engine/Component/ActorComponent.h"
#include "Engine/Component/MeshComponent.h"
#include "Engine/Component/ShapeComponent.h"
#include "Engine/Collision/CollisionSystem.h"
#include "Engine/Core/CollisionTypes.h"
#include "Engine/Core/Logging/Timer.h"
#include "Engine/GameFramework/Actor.h"
#include "Engine/GameFramework/World.h"
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

    // --- Drift Salvage: Collision/Sphere/Mesh 헬퍼 바인딩 ---
    //
    // GetComponent("...")가 UActorComponent*를 반환하므로 다운캐스트는
    // 전역 헬퍼(AsSphere/AsMesh)로 우회한다.
    //
    // 사용 예 (Lua):
    //   local sphere = AsSphere(obj:GetComponent("SphereComponent"))
    //   sphere:GrowRadius(dt)
    //   local actors = sphere:GetActorsInRadius("Resource")
    //   for _, a in ipairs(actors) do a:Destroy() end
    //
    //   local mesh = AsMesh(obj:GetComponent("StaticMeshComponent"))
    //   mesh:SetHighlight(true)
    //
    //   local hit = LineTrace(obj, dx, dy, dz, 100, true)
    //   if hit.bHit and hit.Actor then hit.Actor:AddPosition(dx*5, dy*5, dz*5) end
    void BindCollisionHelpers(sol::state& Lua)
    {
        // ---- USphereComponent ----
        Lua.new_usertype<USphereComponent>(
            "SphereComponent",
            "GetRadius",        &USphereComponent::GetSphereRadius,
            "SetRadius",        &USphereComponent::SetSphereRadius,
            "GrowRadius",       &USphereComponent::GrowRadius,
            "ResetRadius",      &USphereComponent::ResetRadius,
            "GetMinRadius",     &USphereComponent::GetMinRadius,
            "GetMaxRadius",     &USphereComponent::GetMaxRadius,
            "SetMinRadius",     &USphereComponent::SetMinRadius,
            "SetMaxRadius",     &USphereComponent::SetMaxRadius,
            "SetGrowthRate",    &USphereComponent::SetGrowthRate,
            // 회수 가능 액터 수집. TagFilter 비우면 Sphere에 들어온 모든 액터 반환.
            "GetActorsInRadius", [](USphereComponent* Self, const std::string& TagFilter, sol::this_state ts) -> sol::table
            {
                sol::state_view Lua(ts);
                sol::table OutTable = Lua.create_table();
                if (!Self) return OutTable;

                TArray<AActor*> Actors;
                Self->GetActorsInRadius(Actors, FString(TagFilter));
                int32 Index = 1;  // Lua는 1-based
                for (AActor* A : Actors)
                {
                    if (IsUsableActor(A))
                    {
                        OutTable[Index++] = A;
                    }
                }
                return OutTable;
            });

        // ---- UMeshComponent ----
        Lua.new_usertype<UMeshComponent>(
            "MeshComponent",
            "SetHighlight",   &UMeshComponent::SetHighlight,
            "IsHighlighted",  &UMeshComponent::IsHighlighted);

        // ---- 다운캐스트 헬퍼 ----
        // GetComponent가 UActorComponent*를 주므로 sol2가 자동 dispatch 못 한다.
        // 명시적 cast로 변환.
        Lua.set_function("AsSphere", [](UActorComponent* Component) -> USphereComponent*
        {
            return IsUsableComponent(Component) ? Cast<USphereComponent>(Component) : nullptr;
        });
        Lua.set_function("AsMesh", [](UActorComponent* Component) -> UMeshComponent*
        {
            return IsUsableComponent(Component) ? Cast<UMeshComponent>(Component) : nullptr;
        });

        // ---- 전역 LineTrace ----
        // Self의 위치에서 (DirX,DirY,DirZ) 방향으로 Length만큼 광선.
        // Self의 Tag와 같은 액터는 무시 (자기 자신/같은 진영 컴포넌트 제외).
        // bDrawDebug=true면 1프레임 시각화 라인이 추가됨.
        // 반환: {bHit, Distance, Location, Normal, Actor}
        Lua.set_function("LineTrace", [](
            AActor* Self,
            float DirX, float DirY, float DirZ,
            float Length,
            bool bDrawDebug,
            sol::this_state ts) -> sol::table
        {
            sol::state_view Lua(ts);
            sol::table Result = Lua.create_table();
            Result["bHit"] = false;

            if (!IsUsableActor(Self))
            {
                return Result;
            }

            UWorld* World = Self->GetFocusedWorld();
            if (!World)
            {
                return Result;
            }

            const FVector Start = Self->GetActorLocation();
            const FVector Dir = FVector(DirX, DirY, DirZ).GetSafeNormal();
            const FVector End = Start + Dir * Length;

            FHitResult Hit;
            const bool bHit = World->GetCollisionSystem().LineTraceSingle(
                World, Start, End, Hit, Self->GetTag(), bDrawDebug);

            Result["bHit"]     = bHit;
            Result["Distance"] = Hit.Distance;
            Result["Location"] = Hit.Location;
            Result["Normal"]   = Hit.Normal;
            Result["Actor"]    = (bHit && Hit.HitComponent) ? Hit.HitComponent->GetOwner() : nullptr;
            return Result;
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
    BindCollisionHelpers(Lua);
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
