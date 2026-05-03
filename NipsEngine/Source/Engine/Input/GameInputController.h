#pragma once

#include <sol/sol.hpp>
#include "Core/Containers/String.h"

class UWorld;
class FViewportCamera;
class FWindowsWindow;
class InputSystem;

class FGameInputController
{
public:
    void SetWindow(FWindowsWindow* InWindow) { Window = InWindow; }
    void SetCamera(FViewportCamera* InCamera);
    void SetWorld(UWorld* InWorld) { World = InWorld; }
    void SetViewportRect(float InX, float InY, float InWidth, float InHeight);
    void SetScriptPath(const FString& InScriptPath);

    const FString& GetScriptPath() const { return ScriptPath; }

    void Tick(float DeltaTime);
    void Reset();

    void SetMouseCaptured(bool bCaptured);
    bool IsMouseCaptured() const { return bMouseCaptured; }

private:
    template <typename... Args>
    bool CallLuaFunction(const char* FunctionName, Args&&... ArgsToForward)
    {
        if (!bScriptLoaded)
        {
            return false;
        }

        sol::object FunctionObject = ScriptEnvironment[FunctionName];
        if (!FunctionObject.valid() || FunctionObject.get_type() != sol::type::function)
        {
            return false;
        }

        sol::protected_function Function = FunctionObject;
        sol::protected_function_result Result = Function(std::forward<Args>(ArgsToForward)...);
        if (!Result.valid())
        {
            sol::error Error = Result;
            printf("[Lua Input Error] %s (%s): %s\n", FunctionName, LoadedScriptPath.c_str(), Error.what());
            return false;
        }

        return true;
    }

    void EnsureScriptLoaded();
    bool LoadScript();
    void InstallBindings();
    FString ResolveScriptPath() const;
    void TickLuaInput(InputSystem& Input);
    void TickFallbackInput(InputSystem& Input);

    void ApplyFallbackKeyDown(int32 KeyCode);
    void ApplyFallbackMouseMove(float DeltaX, float DeltaY);
    void ApplyFallbackMouseClick(const char* ButtonName, bool bPressed);
    void ApplyPendingMovement(float DeltaTime);
    void UpdateCameraRotation();
    void SyncAnglesFromCamera();

    const char* GetKeyName(int32 KeyCode) const;
    const char* GetMouseButtonName(int32 KeyCode) const;

private:
    FWindowsWindow* Window = nullptr;
    FViewportCamera* Camera = nullptr;
    UWorld* World = nullptr;

    float ViewportX = 0.0f;
    float ViewportY = 0.0f;
    float ViewportWidth = 0.0f;
    float ViewportHeight = 0.0f;
    float CachedLocalMouseX = 0.0f;
    float CachedLocalMouseY = 0.0f;

    float MoveSpeed = 15.0f;
    float LookSensitivity = 0.15f;
    float CurrentDeltaTime = 0.0f;
    float Yaw = 0.0f;
    float Pitch = 0.0f;

    float PendingForward = 0.0f;
    float PendingRight = 0.0f;
    float PendingUp = 0.0f;
    float PendingYaw = 0.0f;
    float PendingPitch = 0.0f;

    bool bMouseCaptured = false;
    bool bScriptLoadAttempted = false;
    bool bScriptLoaded = false;

    FString ScriptPath = "Asset/Scripts/PlayerController.lua";
    FString LoadedScriptPath;
    sol::environment ScriptEnvironment;
};
