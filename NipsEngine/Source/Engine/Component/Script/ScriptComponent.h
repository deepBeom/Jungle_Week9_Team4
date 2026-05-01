#pragma once
#include "Engine/Component/ActorComponent.h"
#include <sol/sol.hpp>

class UScriptComponent : public UActorComponent
{
public:
    void BeginPlay() override;
    void TickComponent(float DeltaTime) override;

    void OnOverlap(AActor* OtherActor);
    void OnHit(AActor* OtherActor);

    void SetScriptPath(const std::string& InPath)
    {
        ScriptPath = InPath;
    }

private:
    std::string ScriptPath;

    std::shared_ptr<FLuaScriptInstance> ScriptInstance;
};
