#include "Core/EnginePCH.h"
#include "Scripting/LuaScriptSubsystem.h"

#include "Engine/Scripting/LuaBinder.h"
#include "Engine/Component/Script/ScriptComponent.h"
#include "Engine/Core/Paths.h"
#include "Engine/GameFramework/Actor.h"
#include <algorithm>
#include <cwctype>
#include <filesystem>

namespace
{
    // Bind commonly used script globals into an already created environment.
    void PopulateInstanceEnvironment(FLuaScriptInstance& Instance, AActor* Owner, UScriptComponent* OwnerComponent)
    {
        Instance.Env["Self"] = Owner;
        Instance.Env["Owner"] = Owner;
        Instance.Env["Component"] = OwnerComponent;
        Instance.Env["DestroySelf"] = [Owner]()
        {
            if (Owner && UObject::IsValid(Owner))
            {
                Owner->Destroy();
            }
        };
    }

    // Recreate environment and then bind globals.
    void RecreateAndPopulateInstanceEnvironment(FLuaScriptInstance& Instance, sol::state& Lua, AActor* Owner, UScriptComponent* OwnerComponent)
    {
        Instance.Env = sol::environment(Lua, sol::create, Lua.globals());
        PopulateInstanceEnvironment(Instance, Owner, OwnerComponent);
    }
}

void FLuaScriptSubsystem::LogFunctionError(const FString& FunctionName, const FString& ScriptPath, const char* ErrorMessage) const
{
    UE_LOG("[Lua Function Error] %s (%s) : %s\n",
        FunctionName.c_str(),
        ScriptPath.c_str(),
        ErrorMessage);
}

void FLuaScriptSubsystem::Initialize()
{
    // Shared VM boot. Per-actor isolation is handled by per-instance environments.
    Lua.open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::math,
        sol::lib::table,
        sol::lib::string,
        sol::lib::coroutine);

    BindEngineTypes();
    BindGlobalFunctions();
    RebuildScriptPathCache();
}

void FLuaScriptSubsystem::Shutdown()
{
    // Drop runtime references owned by subsystem.
    ScriptInstances.clear();
    AvailableScriptPaths.clear();
}

bool FLuaScriptSubsystem::CanInvoke(const std::shared_ptr<FLuaScriptInstance>& Instance) const
{
    if (!Instance || !Instance->bLoaded)
    {
        return false;
    }

    if (Instance->Owner && !UObject::IsValid(Instance->Owner))
    {
        return false;
    }

    if (Instance->OwnerComponent && !UObject::IsValid(Instance->OwnerComponent))
    {
        return false;
    }

    return true;
}

FString FLuaScriptSubsystem::ResolveScriptPath(const FString& ScriptPath) const
{
    return FPaths::ToAbsoluteString(FPaths::ToWide(FPaths::Normalize(ScriptPath)));
}

void FLuaScriptSubsystem::BindEngineTypes()
{
    LuaBinder::BindEngineTypes(Lua);
}

void FLuaScriptSubsystem::BindGlobalFunctions()
{
    LuaBinder::BindGlobalFunctions(Lua);
}

std::shared_ptr<FLuaScriptInstance> FLuaScriptSubsystem::CreateScriptInstance(
    AActor* Owner,
    UScriptComponent* OwnerComponent,
    const FString& ScriptPath)
{
    // One component owns one environment; globals are isolated between actors.
    auto Instance = std::make_shared<FLuaScriptInstance>(Lua);

    Instance->Owner = Owner;
    Instance->OwnerComponent = OwnerComponent;
    Instance->ScriptPath = ScriptPath;

    // Env was created in FLuaScriptInstance constructor; only bind globals here.
    PopulateInstanceEnvironment(*Instance, Owner, OwnerComponent);

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

    Instance->bLoaded = false;

    // Resolve to absolute path to avoid working-directory sensitivity.
    const FString ResolvedScriptPath = ResolveScriptPath(Instance->ScriptPath);
    sol::load_result LoadedScript = Lua.load_file(ResolvedScriptPath);

    if (!LoadedScript.valid())
    {
        sol::error Error = LoadedScript;
        UE_LOG("[Lua Load Error] %s : %s\n", ResolvedScriptPath.c_str(), Error.what());
        return false;
    }

    sol::protected_function ScriptFunc = LoadedScript;
    sol::set_environment(Instance->Env, ScriptFunc);

    sol::protected_function_result Result = ScriptFunc();
    if (!Result.valid())
    {
        sol::error Error = Result;
        UE_LOG("[Lua Runtime Error] %s : %s\n", ResolvedScriptPath.c_str(), Error.what());
        return false;
    }

    Instance->ScriptPath = ResolvedScriptPath;
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
    UScriptComponent* OwnerComponent = Instance->OwnerComponent;
    FString ScriptPath = Instance->ScriptPath;

    // Recreate the environment completely so stale Lua globals/upvalues do not survive reload.
    RecreateAndPopulateInstanceEnvironment(*Instance, Lua, Owner, OwnerComponent);

    Instance->ScriptPath = ScriptPath;
    Instance->bLoaded = false;
    return LoadScript(Instance);
}

void FLuaScriptSubsystem::DestroyScriptInstance(const std::shared_ptr<FLuaScriptInstance>& Instance)
{
    auto It = std::remove(ScriptInstances.begin(), ScriptInstances.end(), Instance);
    ScriptInstances.erase(It, ScriptInstances.end());
}

void FLuaScriptSubsystem::ReloadAllScripts()
{
    for (const std::shared_ptr<FLuaScriptInstance>& Instance : ScriptInstances)
    {
        if (!Instance || !Instance->OwnerComponent || !UObject::IsValid(Instance->OwnerComponent))
        {
            continue;
        }

        ReloadScript(Instance);
    }
}

const TArray<FString>& FLuaScriptSubsystem::GetAvailableScriptPaths(bool bForceRefresh)
{
    if (bForceRefresh || AvailableScriptPaths.empty())
    {
        RebuildScriptPathCache();
    }

    return AvailableScriptPaths;
}

void FLuaScriptSubsystem::RefreshAvailableScriptPaths()
{
    RebuildScriptPathCache();
}

void FLuaScriptSubsystem::RebuildScriptPathCache()
{
    AvailableScriptPaths.clear();

    const std::filesystem::path ScriptRoot = std::filesystem::path(FPaths::AssetDirectoryPath()) / L"Scripts";
    std::error_code Ec;
    if (!std::filesystem::exists(ScriptRoot, Ec))
    {
        return;
    }

    // Editor dropdown cache: keep normalized relative paths under Asset/Scripts.
    for (auto It = std::filesystem::recursive_directory_iterator(ScriptRoot, Ec);
        !Ec && It != std::filesystem::recursive_directory_iterator();
        It.increment(Ec))
    {
        if (!It->is_regular_file(Ec))
        {
            continue;
        }

        FWString Extension = It->path().extension().wstring();
        std::transform(Extension.begin(), Extension.end(), Extension.begin(), towlower);
        if (Extension != L".lua")
        {
            continue;
        }

        const FString RelativePath = FPaths::ToRelativeString(It->path().generic_wstring());
        AvailableScriptPaths.push_back(FPaths::Normalize(RelativePath));
    }

    std::sort(AvailableScriptPaths.begin(), AvailableScriptPaths.end());
    AvailableScriptPaths.erase(
        std::unique(AvailableScriptPaths.begin(), AvailableScriptPaths.end()),
        AvailableScriptPaths.end());
}

bool FLuaScriptSubsystem::HasFunction(std::shared_ptr<FLuaScriptInstance> Instance, const FString& FunctionName) const
{
    if (!CanInvoke(Instance))
    {
        return false;
    }

    sol::object FuncObject = Instance->Env[FunctionName];
    return FuncObject.valid() && FuncObject.get_type() == sol::type::function;
}
