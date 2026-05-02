#include "Core/EnginePCH.h"
#include "Scripting/LuaScriptSubsystem.h"
#include "GameFramework/Actor.h"
#include "Editor/UI/EditorConsoleWidget.h"

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

void FLuaScriptSubsystem::BindEngineTypes()
{
    Lua.new_usertype<AActor>(
        "Actor",

		"GetName", [](AActor& Actor) -> std::string
        { return static_cast<std::string>(Actor.GetName()); },
        "SetActive", &AActor::SetActive,
        "Destroy", &AActor::Destroy,
        "IsValid", &AActor::IsValid);
}

void FLuaScriptSubsystem::BindGlobalFunctions()
{
    Lua.set_function("Log", [](const FString& Message)
                     { UE_LOG("[Lua] %s\n", Message.c_str()); });
}

std::shared_ptr<FLuaScriptInstance> FLuaScriptSubsystem::CreateScriptInstance(
    AActor* Owner,
    const FString& ScriptPath)
{
    auto Instance = std::make_shared<FLuaScriptInstance>(Lua);

    Instance->Owner = Owner;
    Instance->ScriptPath = ScriptPath;

    // Actor별 environment 안에 Self 등록
    Instance->Env["Self"] = Owner;

    // Actor별 helper 함수도 environment에 넣을 수 있음
    Instance->Env["DestroySelf"] = [Owner]()
    {
        if (Owner)// && Owner->IsValid())
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

    sol::load_result LoadedScript = Lua.load_file(Instance->ScriptPath);

    if (!LoadedScript.valid())
    {
        sol::error Error = LoadedScript;
        printf("[Lua Load Error] %s\n", Error.what());
        return false;
    }

    sol::protected_function ScriptFunc = LoadedScript;

    // 핵심: 이 Lua 파일을 Actor별 environment에서 실행시킨다.
    sol::set_environment(Instance->Env, ScriptFunc);

    sol::protected_function_result Result = ScriptFunc();

    if (!Result.valid())
    {
        sol::error Error = Result;
        printf("[Lua Runtime Error] %s\n", Error.what());
        return false;
    }

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
    std::string ScriptPath = Instance->ScriptPath;

    Instance->Env = sol::environment(Lua, sol::create, Lua.globals());

    Instance->Env["Self"] = Owner;

    Instance->Env["DestroySelf"] = [Owner]()
    {
        if (Owner)// && Owner->IsValid())
        {
            Owner->Destroy();
        }
    };

    Instance->ScriptPath = ScriptPath;
    Instance->bLoaded = false;

    return LoadScript(Instance);
}
