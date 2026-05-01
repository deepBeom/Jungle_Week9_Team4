#include "Core/EnginePCH.h"
#include "Scripting/LuaScriptSubsystem.h"

#include "Engine/Component/ActorComponent.h"
#include "Engine/Component/Script/ScriptComponent.h"
#include "Engine/Core/CollisionTypes.h"
#include "Engine/Core/Logging/Timer.h"
#include "Engine/Core/Paths.h"
#include "Engine/GameFramework/Actor.h"
#include "Engine/Runtime/Engine.h"

void FLuaScriptSubsystem::LogFunctionError(const std::string& FunctionName, const FString& ScriptPath, const char* ErrorMessage) const
{
    printf("[Lua Function Error] %s (%s) : %s\n",
        FunctionName.c_str(),
        ScriptPath.c_str(),
        ErrorMessage);
}

void FLuaScriptSubsystem::Initialize()
{
    Lua.open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::math,
        sol::lib::table,
        sol::lib::string,
        sol::lib::coroutine);

    BindEngineTypes();
    BindGlobalFunctions();
}

void FLuaScriptSubsystem::Shutdown()
{
    ScriptInstances.clear();
}

bool FLuaScriptSubsystem::CanInvoke(const std::shared_ptr<FLuaScriptInstance>& Instance) const
{
    if (!Instance || !Instance->bLoaded)
    {
        return false;
    }

    if (Instance->Owner && !UObject::IsValid(Instance->Owner))
    {
        return false;
    }

    if (Instance->OwnerComponent && !UObject::IsValid(Instance->OwnerComponent))
    {
        return false;
    }

    return true;
}

FString FLuaScriptSubsystem::ResolveScriptPath(const FString& ScriptPath) const
{
    return FPaths::ToAbsoluteString(FPaths::ToWide(FPaths::Normalize(ScriptPath)));
}

void FLuaScriptSubsystem::BindEngineTypes()
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

    Lua.new_usertype<UActorComponent>(
        "Component",
        "GetName", [](UActorComponent* Component) -> std::string
        {
            return (Component && UObject::IsValid(Component))
                ? static_cast<std::string>(Component->GetName())
                : std::string();
        },
        "GetTypeName", [](UActorComponent* Component) -> std::string
        {
            return (Component && UObject::IsValid(Component))
                ? Component->GetTypeInfo()->name
                : std::string();
        },
        "GetOwner", [](UActorComponent* Component) -> AActor*
        {
            if (!Component || !UObject::IsValid(Component))
            {
                return nullptr;
            }

            AActor* Owner = Component->GetOwner();
            if (!Owner || !UObject::IsValid(Owner) || Owner->IsPendingDestroy())
            {
                return nullptr;
            }

            return Owner;
        },
        "SetActive", [](UActorComponent* Component, bool bEnabled)
        {
            if (Component && UObject::IsValid(Component))
            {
                Component->SetActive(bEnabled);
            }
        },
        "IsActive", [](UActorComponent* Component)
        {
            return Component && UObject::IsValid(Component) && Component->IsActive();
        });

    Lua.new_usertype<AActor>(
        "Actor",
        "GetName", [](AActor* Actor) -> std::string
        {
            return (Actor && UObject::IsValid(Actor))
                ? static_cast<std::string>(Actor->GetName())
                : std::string();
        },
        "GetPosition", [](AActor* Actor)
        {
            return (Actor && UObject::IsValid(Actor) && !Actor->IsPendingDestroy())
                ? Actor->GetActorLocation()
                : FVector::ZeroVector;
        },
        "SetPosition", [](AActor* Actor, float X, float Y, float Z)
        {
            if (Actor && UObject::IsValid(Actor) && !Actor->IsPendingDestroy())
            {
                Actor->SetActorLocation(FVector(X, Y, Z));
            }
        },
        "AddPosition", [](AActor* Actor, float X, float Y, float Z)
        {
            if (Actor && UObject::IsValid(Actor) && !Actor->IsPendingDestroy())
            {
                Actor->AddActorWorldOffset(FVector(X, Y, Z));
            }
        },
        "GetRotation", [](AActor* Actor)
        {
            return (Actor && UObject::IsValid(Actor) && !Actor->IsPendingDestroy())
                ? Actor->GetActorRotation()
                : FVector::ZeroVector;
        },
        "SetRotation", [](AActor* Actor, float X, float Y, float Z)
        {
            if (Actor && UObject::IsValid(Actor) && !Actor->IsPendingDestroy())
            {
                Actor->SetActorRotation(FVector(X, Y, Z));
            }
        },
        "GetScale", [](AActor* Actor)
        {
            return (Actor && UObject::IsValid(Actor) && !Actor->IsPendingDestroy())
                ? Actor->GetActorScale()
                : FVector(1.0f, 1.0f, 1.0f);
        },
        "SetScale", [](AActor* Actor, float X, float Y, float Z)
        {
            if (Actor && UObject::IsValid(Actor) && !Actor->IsPendingDestroy())
            {
                Actor->SetActorScale(FVector(X, Y, Z));
            }
        },
        "GetForwardVector", [](AActor* Actor)
        {
            return (Actor && UObject::IsValid(Actor) && !Actor->IsPendingDestroy())
                ? Actor->GetActorForward()
                : FVector(0.0f, 0.0f, 1.0f);
        },
        "SetActive", [](AActor* Actor, bool bEnabled)
        {
            if (Actor && UObject::IsValid(Actor) && !Actor->IsPendingDestroy())
            {
                Actor->SetActive(bEnabled);
            }
        },
        "Destroy", [](AActor* Actor)
        {
            if (Actor && UObject::IsValid(Actor))
            {
                Actor->Destroy();
            }
        },
        "GetComponent", [](AActor* Actor, const std::string& TypeName) -> UActorComponent*
        {
            if (!Actor || !UObject::IsValid(Actor) || Actor->IsPendingDestroy())
            {
                return nullptr;
            }

            const FString RequestedType = TypeName;
            const FString PrefixedType = RequestedType.rfind("U", 0) == 0
                ? RequestedType
                : ("U" + RequestedType);

            for (UActorComponent* Component : Actor->GetComponents())
            {
                if (!Component || !UObject::IsValid(Component))
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
        },
        "FindComponentByClass", [](AActor* Actor, const std::string& TypeName) -> UActorComponent*
        {
            if (!Actor || !UObject::IsValid(Actor) || Actor->IsPendingDestroy())
            {
                return nullptr;
            }

            const FString RequestedType = TypeName;
            const FString PrefixedType = RequestedType.rfind("U", 0) == 0
                ? RequestedType
                : ("U" + RequestedType);

            for (UActorComponent* Component : Actor->GetComponents())
            {
                if (!Component || !UObject::IsValid(Component))
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
        },
        "IsPendingDestroy", [](AActor* Actor)
        {
            return Actor && UObject::IsValid(Actor) && Actor->IsPendingDestroy();
        },
        "IsValid", [](AActor* Actor)
        {
            return Actor && UObject::IsValid(Actor) && !Actor->IsPendingDestroy();
        });
}

void FLuaScriptSubsystem::BindGlobalFunctions()
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

std::shared_ptr<FLuaScriptInstance> FLuaScriptSubsystem::CreateScriptInstance(
    AActor* Owner,
    UScriptComponent* OwnerComponent,
    const FString& ScriptPath)
{
    auto Instance = std::make_shared<FLuaScriptInstance>(Lua);

    Instance->Owner = Owner;
    Instance->OwnerComponent = OwnerComponent;
    Instance->ScriptPath = ScriptPath;

    Instance->Env["Self"] = Owner;
    Instance->Env["Owner"] = Owner;
    Instance->Env["Component"] = OwnerComponent;
    Instance->Env["DestroySelf"] = [Owner]()
    {
        if (Owner && UObject::IsValid(Owner))
        {
            Owner->Destroy();
        }
    };

    ScriptInstances.push_back(Instance);
    LoadScript(Instance);
    return Instance;
}

bool FLuaScriptSubsystem::LoadScript(std::shared_ptr<FLuaScriptInstance> Instance)
{
    if (!Instance)
    {
        return false;
    }

    Instance->bLoaded = false;

    const FString ResolvedScriptPath = ResolveScriptPath(Instance->ScriptPath);
    sol::load_result LoadedScript = Lua.load_file(ResolvedScriptPath);

    if (!LoadedScript.valid())
    {
        sol::error Error = LoadedScript;
        printf("[Lua Load Error] %s : %s\n", ResolvedScriptPath.c_str(), Error.what());
        return false;
    }

    sol::protected_function ScriptFunc = LoadedScript;
    sol::set_environment(Instance->Env, ScriptFunc);

    sol::protected_function_result Result = ScriptFunc();
    if (!Result.valid())
    {
        sol::error Error = Result;
        printf("[Lua Runtime Error] %s : %s\n", ResolvedScriptPath.c_str(), Error.what());
        return false;
    }

    Instance->ScriptPath = ResolvedScriptPath;
    Instance->bLoaded = true;
    return true;
}

bool FLuaScriptSubsystem::ReloadScript(std::shared_ptr<FLuaScriptInstance> Instance)
{
    if (!Instance)
    {
        return false;
    }

    AActor* Owner = Instance->Owner;
    UScriptComponent* OwnerComponent = Instance->OwnerComponent;
    FString ScriptPath = Instance->ScriptPath;

    Instance->Env = sol::environment(Lua, sol::create, Lua.globals());
    Instance->Env["Self"] = Owner;
    Instance->Env["Owner"] = Owner;
    Instance->Env["Component"] = OwnerComponent;
    Instance->Env["DestroySelf"] = [Owner]()
    {
        if (Owner && UObject::IsValid(Owner))
        {
            Owner->Destroy();
        }
    };

    Instance->ScriptPath = ScriptPath;
    Instance->bLoaded = false;
    return LoadScript(Instance);
}

void FLuaScriptSubsystem::DestroyScriptInstance(const std::shared_ptr<FLuaScriptInstance>& Instance)
{
    auto It = std::remove(ScriptInstances.begin(), ScriptInstances.end(), Instance);
    ScriptInstances.erase(It, ScriptInstances.end());
}

void FLuaScriptSubsystem::ReloadAllScripts()
{
    for (const std::shared_ptr<FLuaScriptInstance>& Instance : ScriptInstances)
    {
        if (!Instance || !Instance->OwnerComponent || !UObject::IsValid(Instance->OwnerComponent))
        {
            continue;
        }

        ReloadScript(Instance);
    }
}

bool FLuaScriptSubsystem::HasFunction(std::shared_ptr<FLuaScriptInstance> Instance, const std::string& FunctionName) const
{
    if (!CanInvoke(Instance))
    {
        return false;
    }

    sol::object FuncObject = Instance->Env[FunctionName];
    return FuncObject.valid() && FuncObject.get_type() == sol::type::function;
}
