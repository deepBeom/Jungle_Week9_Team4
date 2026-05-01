#include "Core/EnginePCH.h"
#include "Engine/Component/Script/ScriptComponent.h"

#include "Engine/Core/CollisionTypes.h"
#include "Engine/Core/Paths.h"
#include "Engine/Runtime/Engine.h"
#include "Engine/Scripting/LuaScriptSubsystem.h"

DEFINE_CLASS(UScriptComponent, UActorComponent)
REGISTER_FACTORY(UScriptComponent)

void UScriptComponent::BeginPlay()
{
    bHasBegunPlay = true;
    bDestroyNotified = false;
    bEnableNotified = false;
    ConsecutiveRuntimeErrorCount = 0;

    if (!EnsureScriptInstance())
    {
        return;
    }

    if (IsActive())
    {
        StartScriptIfNeeded();
        if (IsActive())
        {
            NotifyScriptEnabled();
        }
    }
}

void UScriptComponent::EndPlay()
{
    NotifyScriptDestroyed();

    if (ScriptInstance)
    {
        GEngine->GetLuaScriptSubsystem().DestroyScriptInstance(ScriptInstance);
        ScriptInstance.reset();
    }

    bHasBegunPlay = false;
    bStarted = false;
    bEnableNotified = false;
    ConsecutiveRuntimeErrorCount = 0;
}

void UScriptComponent::TickComponent(float DeltaTime)
{
    if (!bHasBegunPlay || !IsActive() || !ScriptInstance || !ScriptInstance->bLoaded)
    {
        return;
    }

    StartScriptIfNeeded();
    if (!IsActive())
    {
        return;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    const bool bHasUpdateCallback = LuaSubsystem.HasFunction(ScriptInstance, "OnUpdate")
        || LuaSubsystem.HasFunction(ScriptInstance, "Tick");
    const bool bResult = LuaSubsystem.HasFunction(ScriptInstance, "OnUpdate")
        ? LuaSubsystem.CallFunction(ScriptInstance, "OnUpdate", GetOwner(), DeltaTime)
        : TryCallPreferredFloat({ "Tick" }, DeltaTime);
    HandleRuntimeCallbackResult(bResult, bHasUpdateCallback);
}

void UScriptComponent::Activate()
{
    UActorComponent::Activate();
    bSerializedEnabled = true;

    if (!bHasBegunPlay)
    {
        return;
    }

    if (!EnsureScriptInstance())
    {
        return;
    }

    StartScriptIfNeeded();
    if (IsActive())
    {
        NotifyScriptEnabled();
    }
}

void UScriptComponent::Deactivate()
{
    if (bHasBegunPlay)
    {
        NotifyScriptDisabled();
    }

    bSerializedEnabled = false;
    UActorComponent::Deactivate();
}

void UScriptComponent::Serialize(FArchive& Ar)
{
    UActorComponent::Serialize(Ar);
    Ar << "ScriptPath" << ScriptPath;
    Ar << "ScriptEnabled" << bSerializedEnabled;

    if (Ar.IsLoading())
    {
        ScriptPath = FPaths::Normalize(ScriptPath);
        SetActive(bSerializedEnabled);
    }
}

void UScriptComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
    UActorComponent::GetEditableProperties(OutProps);
    OutProps.push_back({ "Script Path", EPropertyType::String, &ScriptPath });
    OutProps.push_back({ "Script Enabled", EPropertyType::Bool, &bSerializedEnabled });
}

void UScriptComponent::PostEditProperty(const char* PropertyName)
{
    UActorComponent::PostEditProperty(PropertyName);

    if (strcmp(PropertyName, "Script Path") == 0)
    {
        ScriptPath = FPaths::Normalize(ScriptPath);
        if (bHasBegunPlay)
        {
            ReloadScript();
        }
        return;
    }

    if (strcmp(PropertyName, "Script Enabled") == 0)
    {
        SetActive(bSerializedEnabled);
    }
}

void UScriptComponent::OnOverlap(AActor* OtherActor)
{
    OnOverlapBegin(OtherActor);
}

void UScriptComponent::OnOverlapBegin(AActor* OtherActor)
{
    if (!ScriptInstance || !ScriptInstance->bLoaded)
    {
        return;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    const bool bHasCallback = LuaSubsystem.HasFunction(ScriptInstance, "OnOverlapBegin")
        || LuaSubsystem.HasFunction(ScriptInstance, "OnOverlap");
    const bool bResult = TryCallPreferredActor({ "OnOverlapBegin", "OnOverlap" }, OtherActor);
    HandleRuntimeCallbackResult(bResult, bHasCallback);
}

void UScriptComponent::OnOverlapEnd(AActor* OtherActor)
{
    if (!ScriptInstance || !ScriptInstance->bLoaded)
    {
        return;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    const bool bHasCallback = LuaSubsystem.HasFunction(ScriptInstance, "OnOverlapEnd");
    const bool bResult = TryCallPreferredActor({ "OnOverlapEnd" }, OtherActor);
    HandleRuntimeCallbackResult(bResult, bHasCallback);
}

void UScriptComponent::OnHit(AActor* OtherActor)
{
    FHitResult EmptyHitInfo;
    OnHit(OtherActor, EmptyHitInfo);
}

void UScriptComponent::OnHit(AActor* OtherActor, const FHitResult& HitInfo)
{
    if (!ScriptInstance || !ScriptInstance->bLoaded)
    {
        return;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    const bool bHasCallback = LuaSubsystem.HasFunction(ScriptInstance, "OnHit");
    const bool bResult = TryCallPreferredHit({ "OnHit" }, OtherActor, HitInfo);
    HandleRuntimeCallbackResult(bResult, bHasCallback);
}

bool UScriptComponent::ReloadScript()
{
    if (!ScriptInstance)
    {
        return EnsureScriptInstance();
    }

    NotifyScriptDestroyed();

    const bool bReloaded = GEngine->GetLuaScriptSubsystem().ReloadScript(ScriptInstance);
    bStarted = false;
    bEnableNotified = false;
    bDestroyNotified = false;
    ConsecutiveRuntimeErrorCount = 0;

    if (!bReloaded)
    {
        DisableScriptAfterFatalError();
        return false;
    }

    if (bHasBegunPlay && IsActive())
    {
        StartScriptIfNeeded();
        if (IsActive())
        {
            NotifyScriptEnabled();
        }
    }

    return true;
}

void UScriptComponent::SetScriptPath(const std::string& InPath)
{
    ScriptPath = FPaths::Normalize(InPath);
}

bool UScriptComponent::IsScriptLoaded() const
{
    return ScriptInstance && ScriptInstance->bLoaded;
}

bool UScriptComponent::EnsureScriptInstance()
{
    if (ScriptPath.empty())
    {
        return false;
    }

    if (ScriptInstance)
    {
        return ScriptInstance->bLoaded;
    }

    ScriptInstance = GEngine->GetLuaScriptSubsystem().CreateScriptInstance(GetOwner(), this, ScriptPath);
    if (!ScriptInstance || !ScriptInstance->bLoaded)
    {
        DisableScriptAfterFatalError();
        return false;
    }

    return true;
}

void UScriptComponent::StartScriptIfNeeded()
{
    if (bStarted || !ScriptInstance || !ScriptInstance->bLoaded)
    {
        return;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    const bool bHasStartCallback = LuaSubsystem.HasFunction(ScriptInstance, "OnStart")
        || LuaSubsystem.HasFunction(ScriptInstance, "BeginPlay");
    const bool bResult = TryCallPreferred({ "OnStart", "BeginPlay" });
    if (!bResult && bHasStartCallback)
    {
        DisableScriptAfterFatalError();
        return;
    }

    bStarted = true;
}

void UScriptComponent::NotifyScriptEnabled()
{
    if (bEnableNotified || !ScriptInstance || !ScriptInstance->bLoaded)
    {
        return;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    const bool bHasEnableCallback = LuaSubsystem.HasFunction(ScriptInstance, "OnEnable");
    const bool bResult = TryCallPreferred({ "OnEnable" });
    if (!bResult && bHasEnableCallback)
    {
        HandleRuntimeCallbackResult(false, true);
        return;
    }

    HandleRuntimeCallbackResult(true, bHasEnableCallback);
    bEnableNotified = true;
}

void UScriptComponent::NotifyScriptDisabled()
{
    if (!bEnableNotified || !ScriptInstance || !ScriptInstance->bLoaded)
    {
        bEnableNotified = false;
        return;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    const bool bHasDisableCallback = LuaSubsystem.HasFunction(ScriptInstance, "OnDisable");
    const bool bResult = TryCallPreferred({ "OnDisable" });
    if (!bResult && bHasDisableCallback)
    {
        HandleRuntimeCallbackResult(false, true);
        return;
    }

    HandleRuntimeCallbackResult(true, bHasDisableCallback);
    bEnableNotified = false;
}

void UScriptComponent::NotifyScriptDestroyed()
{
    if (bDestroyNotified || !ScriptInstance || !ScriptInstance->bLoaded)
    {
        return;
    }

    NotifyScriptDisabled();

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    const bool bHasDestroyCallback = LuaSubsystem.HasFunction(ScriptInstance, "OnDestroy");
    const bool bResult = TryCallPreferred({ "OnDestroy" });
    if (!bResult && bHasDestroyCallback)
    {
        HandleRuntimeCallbackResult(false, true);
        return;
    }

    HandleRuntimeCallbackResult(true, bHasDestroyCallback);
    bDestroyNotified = true;
}

bool UScriptComponent::TryCallPreferred(const std::initializer_list<const char*>& CallbackNames)
{
    if (!ScriptInstance || !ScriptInstance->bLoaded)
    {
        return false;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    for (const char* CallbackName : CallbackNames)
    {
        if (!LuaSubsystem.HasFunction(ScriptInstance, CallbackName))
        {
            continue;
        }

        return LuaSubsystem.CallFunction(ScriptInstance, CallbackName, GetOwner());
    }

    return true;
}

bool UScriptComponent::TryCallPreferredActor(const std::initializer_list<const char*>& CallbackNames, AActor* OtherActor)
{
    if (!ScriptInstance || !ScriptInstance->bLoaded)
    {
        return false;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    for (const char* CallbackName : CallbackNames)
    {
        if (!LuaSubsystem.HasFunction(ScriptInstance, CallbackName))
        {
            continue;
        }

        return LuaSubsystem.CallFunction(ScriptInstance, CallbackName, GetOwner(), OtherActor);
    }

    return true;
}

bool UScriptComponent::TryCallPreferredFloat(const std::initializer_list<const char*>& CallbackNames, float Value)
{
    if (!ScriptInstance || !ScriptInstance->bLoaded)
    {
        return false;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    for (const char* CallbackName : CallbackNames)
    {
        if (!LuaSubsystem.HasFunction(ScriptInstance, CallbackName))
        {
            continue;
        }

        return LuaSubsystem.CallFunction(ScriptInstance, CallbackName, GetOwner(), Value);
    }

    return true;
}

bool UScriptComponent::TryCallPreferredHit(const std::initializer_list<const char*>& CallbackNames, AActor* OtherActor, const FHitResult& HitInfo)
{
    if (!ScriptInstance || !ScriptInstance->bLoaded)
    {
        return false;
    }

    FLuaScriptSubsystem& LuaSubsystem = GEngine->GetLuaScriptSubsystem();
    for (const char* CallbackName : CallbackNames)
    {
        if (!LuaSubsystem.HasFunction(ScriptInstance, CallbackName))
        {
            continue;
        }

        return LuaSubsystem.CallFunction(ScriptInstance, CallbackName, GetOwner(), OtherActor, HitInfo);
    }

    return true;
}

void UScriptComponent::DisableScriptAfterFatalError()
{
    bIsActive = false;
    bSerializedEnabled = false;
    bCanEverTick = false;
    bEnableNotified = false;
}

void UScriptComponent::HandleRuntimeCallbackResult(bool bSucceeded, bool bHadCallback)
{
    if (!bHadCallback)
    {
        return;
    }

    if (bSucceeded)
    {
        ConsecutiveRuntimeErrorCount = 0;
        return;
    }

    ++ConsecutiveRuntimeErrorCount;
    if (ConsecutiveRuntimeErrorCount >= 3)
    {
        printf("[Lua] Disabling script after %d consecutive callback failures: %s\n",
            ConsecutiveRuntimeErrorCount,
            ScriptPath.c_str());
        DisableScriptAfterFatalError();
    }
}
