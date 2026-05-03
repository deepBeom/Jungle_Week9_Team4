#pragma once

#include "Core/Containers/String.h"
#include "Math/Vector.h"

#include <sol/sol.hpp>

/**
 * @brief Centralized Lua binding registration helpers.
 * @ingroup LuaScriptingBinding
 *
 * This module owns only type/global registration logic.
 * Runtime instance ownership and script load/reload remain in FLuaScriptSubsystem.
 */
namespace LuaBinder
{
    void SetGameplayInputEnabled(bool bEnabled);
    bool IsGameplayInputEnabled();
    void SetGameplayCameraFollowEnabled(bool bEnabled);
    bool IsGameplayCameraFollowEnabled();
    void SetGameplayCameraLookAt(const FVector& Location, const FVector& Target);
    void SetGameplayCameraLookAt(float LocationX, float LocationY, float LocationZ, float TargetX, float TargetY, float TargetZ);
    bool GetGameplayCameraLookAt(FVector& OutLocation, FVector& OutTarget);
    void SetGameplayCameraTransform(const FVector& Location, const FVector& RotationEuler);
    void SetGameplayCameraTransform(float LocationX, float LocationY, float LocationZ, float RotationX, float RotationY, float RotationZ);
    bool GetGameplayCameraTransform(FVector& OutLocation, FVector& OutRotationEuler);
    void ResetDriftSalvageStats();
    void ApplyDriftSalvageDamage(int32 Damage);
    void ApplyDriftSalvagePickup(const FString& ActorTag);
    int32 GetDriftSalvageHealth();
    int32 GetDriftSalvageMoney();
    float GetDriftSalvageWeight();
    float GetDriftSalvageWeightCapacity();

    /**
     * @brief Registers engine-facing userdata/types (Vec3, HitInfo, Component, Actor).
     * @param Lua Shared Lua state used by the scripting subsystem.
     */
    void BindEngineTypes(sol::state& Lua);

    /**
     * @brief Registers global helper functions (Log, Warning, Error, Time, FrameCount).
     * @param Lua Shared Lua state used by the scripting subsystem.
     */
    void BindGlobalFunctions(sol::state& Lua);
}
