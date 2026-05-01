#pragma once
#include "Engine/Component/ActorComponent.h"
#include "sol2/include/sol/sol.hpp"

class UScriptComponent : public UActorComponent
{
public:
    void BeginPlay() override;
    void Tick(float DeltaTime);

    void HandleOverlap(AActor* OtherActor);

private:
    std::string ScriptPath;

    sol::environment Env;
    sol::protected_function LuaOnBeginPlay;
    sol::protected_function LuaOnTick;
    sol::protected_function LuaOnOverlap;
};
