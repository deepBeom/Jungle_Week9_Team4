#pragma once

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
