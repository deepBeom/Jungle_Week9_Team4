#include "Core/EnginePCH.h"
#include "Engine/Runtime/Engine.h"
#include "Engine/Component/Script/ScriptComponent.h"
#include "Engine/Scripting/LuaScriptSubsystem.h"

void UScriptComponent::BeginPlay()
{
    AActor* Owner = GetOwner();

    FLuaScriptSubsystem& LuaSubsystem =
        GEngine->GetLuaScriptSubsystem();

    ScriptInstance = LuaSubsystem.CreateScriptInstance(
        Owner,
        ScriptPath);

    LuaSubsystem.CallFunction(ScriptInstance, "BeginPlay");
}

void UScriptComponent::TickComponent(float DeltaTime)
{
    if (!ScriptInstance)
    {
        return;
    }

    FLuaScriptSubsystem& LuaSubsystem =
        GEngine->GetLuaScriptSubsystem();

    LuaSubsystem.CallFunction(ScriptInstance, "Tick", DeltaTime);
}

void UScriptComponent::OnOverlap(AActor* OtherActor)
{
    if (!ScriptInstance)
    {
        return;
    }

    FLuaScriptSubsystem& LuaSubsystem =
        GEngine->GetLuaScriptSubsystem();

    LuaSubsystem.CallFunction(ScriptInstance, "OnOverlap", OtherActor);
}

void UScriptComponent::OnHit(AActor* OtherActor)
{
    if (!ScriptInstance)
    {
        return;
    }

    FLuaScriptSubsystem& LuaSubsystem =
        GEngine->GetLuaScriptSubsystem();

    LuaSubsystem.CallFunction(ScriptInstance, "OnHit", OtherActor);
}