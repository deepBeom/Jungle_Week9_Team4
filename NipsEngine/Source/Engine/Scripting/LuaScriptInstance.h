#pragma once

#include <sol/sol.hpp>

class AActor;

struct FLuaScriptInstance
{
    AActor* Owner = nullptr;
    FString ScriptPath;

    // Actor별 Lua 전역 공간
    sol::environment Env;

    bool bLoaded = false;

    FLuaScriptInstance(sol::state& Lua)
        : Env(Lua, sol::create, Lua.globals())
    {
    }
};
