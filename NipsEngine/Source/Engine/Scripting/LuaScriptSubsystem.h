#pragma once

#include <sol/sol.hpp>
#include "Engine/Scripting/LuaScriptInstance.h"

class AActor;

class FLuaScriptSubsystem
{
public:
    void Initialize();
    void Shutdown();

    std::shared_ptr<FLuaScriptInstance> CreateScriptInstance(
        AActor* Owner,
        const FString& ScriptPath);

    bool LoadScript(std::shared_ptr<FLuaScriptInstance> Instance);
    bool ReloadScript(std::shared_ptr<FLuaScriptInstance> Instance);

	template <typename... Args>
    bool CallFunction(
        std::shared_ptr<FLuaScriptInstance> Instance,
        const std::string& FunctionName,
        Args&&... args)
    {
        if (!Instance || !Instance->bLoaded)
        {
            return false;
        }

        sol::object FuncObject = Instance->Env[FunctionName];

        if (!FuncObject.valid() || FuncObject.get_type() != sol::type::function)
        {
            // 함수가 없는 것은 에러가 아닐 수도 있다.
            // 예: OnHit을 구현하지 않은 스크립트
            return false;
        }

        sol::protected_function Func = FuncObject;

        sol::protected_function_result Result =
            Func(std::forward<Args>(args)...);

        if (!Result.valid())
        {
            sol::error Error = Result;
            printf("[Lua Function Error] %s\n", Error.what());
            return false;
        }

        return true;
    }

    sol::state& GetLuaState() { return Lua; }

private:
    void BindEngineTypes();
    void BindGlobalFunctions();

private:
    sol::state Lua;

    TArray<std::shared_ptr<FLuaScriptInstance>> ScriptInstances;
};
