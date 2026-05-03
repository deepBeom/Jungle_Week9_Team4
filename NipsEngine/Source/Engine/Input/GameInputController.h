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

    void SetCursorHidden(bool bHidden);
    void SetMouseLocked(bool bLocked);
    void SetCursorHiddenAllowed(bool bAllowed);
    void SetMouseLockAllowed(bool bAllowed);

    bool IsCursorHidden() const { return bCursorHidden; }
    bool IsMouseLocked() const { return bMouseLocked; }

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
    void ApplyCursorVisibilityState();
    void ApplyMouseLockState();

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

    bool bCursorHidden = false;               // 실제 커서 숨김 상태입니다.
    bool bMouseLocked = false;                // 실제 마우스 고정 상태입니다.
    bool bRequestsCursorHidden = false;       // Lua/게임플레이가 커서 숨김을 원하는 상태입니다.
    bool bRequestsMouseLock = false;          // Lua/게임플레이가 마우스 고정을 원하는 상태입니다.
    bool bAllowsCursorHidden = true;          // PIE/에디터 상태에서 커서 숨김을 적용할 수 있는지 나타냅니다.
    bool bAllowsMouseLock = true;             // PIE/에디터 상태에서 마우스 고정을 적용할 수 있는지 나타냅니다.
    bool bScriptLoadAttempted = false;
    bool bScriptLoaded = false;

    FString ScriptPath = "Asset/Scripts/PlayerController.lua";
    FString LoadedScriptPath;
    sol::environment ScriptEnvironment;
};
