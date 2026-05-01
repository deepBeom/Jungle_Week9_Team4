#pragma once

#include <sol/sol.hpp>

class AActor;
class UScriptComponent;

struct FLuaScriptInstance
{
    AActor* Owner = nullptr;
    UScriptComponent* OwnerComponent = nullptr;
    FString ScriptPath;

    // Actor별 Lua 전역 공간
    sol::environment Env;

    bool bLoaded = false;

    FLuaScriptInstance(sol::state& Lua)
        : Env(Lua, sol::create, Lua.globals())
    {
    }
};
