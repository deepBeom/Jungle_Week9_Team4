#include "Engine/Input/GameInputController.h"

#include <cmath>
#include "Engine/Component/CameraComponent.h"
#include "Engine/Component/SceneComponent.h"
#include "Engine/Core/Paths.h"
#include "Engine/GameFramework/Pawn.h"
#include "Engine/GameFramework/World.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Math/Utils.h"
#include "Engine/Runtime/Engine.h"
#include "Engine/Runtime/WindowsWindow.h"
#include "Engine/Scripting/LuaBinder.h"
#include "Engine/Viewport/ViewportCamera.h"

namespace
{
    constexpr int32 WatchedKeys[] = { 'W', 'A', 'S', 'D', 'Q', 'E' };
    constexpr int32 WatchedMouseButtons[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
}

void FGameInputController::SetCamera(FViewportCamera* InCamera)
{
    Camera = InCamera;
    SyncAnglesFromCamera();
}

void FGameInputController::SetViewportRect(float InX, float InY, float InWidth, float InHeight)
{
    ViewportX = InX;
    ViewportY = InY;
    ViewportWidth = InWidth;
    ViewportHeight = InHeight;
}

void FGameInputController::SetScriptPath(const FString& InScriptPath)
{
    ScriptPath = InScriptPath;
    bScriptLoadAttempted = false;
    bScriptLoaded = false;
    LoadedScriptPath.clear();
    ScriptEnvironment = sol::environment();
}

void FGameInputController::Tick(float DeltaTime)
{
    CurrentDeltaTime = DeltaTime;
    PendingForward = 0.0f;
    PendingRight = 0.0f;
    PendingUp = 0.0f;
    PendingYaw = 0.0f;
    PendingPitch = 0.0f;

    if (!Camera)
    {
        return;
    }

    RefreshControlledPawn();
    SyncViewportCameraFromPawn();

    EnsureScriptLoaded();

    InputSystem& Input = InputSystem::Get();

    if (!LuaBinder::IsGameplayInputEnabled())
    {
        SetCursorHidden(false);
        SetMouseLocked(false);
        return;
    }

    SetCursorHidden(true);
    SetMouseLocked(true);

    POINT MousePoint = Input.GetMousePos();
    if (Window)
    {
        MousePoint = Window->ScreenToClientPoint(MousePoint);
    }

    CachedLocalMouseX = static_cast<float>(MousePoint.x) - ViewportX;
    CachedLocalMouseY = static_cast<float>(MousePoint.y) - ViewportY;

    if (bScriptLoaded)
    {
        TickLuaInput(Input);
    }
    else
    {
        TickFallbackInput(Input);
    }

    ApplyPendingMovement(DeltaTime);
    SyncViewportCameraFromPawn();
}

void FGameInputController::TickLuaInput(InputSystem& Input)
{
    for (int32 KeyCode : WatchedKeys)
    {
        if (Input.GetKey(KeyCode))
        {
            CallLuaFunction("OnKeyDown", GetKeyName(KeyCode), KeyCode);
        }

        if (Input.GetKeyUp(KeyCode))
        {
            CallLuaFunction("OnKeyUp", GetKeyName(KeyCode), KeyCode);
        }
    }

    if (Input.MouseMoved())
    {
        const float DeltaX = static_cast<float>(Input.MouseDeltaX());
        const float DeltaY = static_cast<float>(Input.MouseDeltaY());
        CallLuaFunction("OnMouseMove", DeltaX, DeltaY, CachedLocalMouseX, CachedLocalMouseY);
    }

    for (int32 MouseButton : WatchedMouseButtons)
    {
        if (Input.GetKeyDown(MouseButton))
        {
            const char* ButtonName = GetMouseButtonName(MouseButton);
            CallLuaFunction("OnMouseClick", ButtonName, true, CachedLocalMouseX, CachedLocalMouseY);
        }

        if (Input.GetKeyUp(MouseButton))
        {
            const char* ButtonName = GetMouseButtonName(MouseButton);
            CallLuaFunction("OnMouseClick", ButtonName, false, CachedLocalMouseX, CachedLocalMouseY);
        }
    }
}

void FGameInputController::TickFallbackInput(InputSystem& Input)
{
    for (int32 KeyCode : WatchedKeys)
    {
        if (Input.GetKey(KeyCode))
        {
            ApplyFallbackKeyDown(KeyCode);
        }
    }

    if (Input.MouseMoved())
    {
        const float DeltaX = static_cast<float>(Input.MouseDeltaX());
        const float DeltaY = static_cast<float>(Input.MouseDeltaY());
        ApplyFallbackMouseMove(DeltaX, DeltaY);
    }

    for (int32 MouseButton : WatchedMouseButtons)
    {
        const char* ButtonName = GetMouseButtonName(MouseButton);
        if (Input.GetKeyDown(MouseButton))
        {
            ApplyFallbackMouseClick(ButtonName, true);
        }

        if (Input.GetKeyUp(MouseButton))
        {
            ApplyFallbackMouseClick(ButtonName, false);
        }
    }
}

void FGameInputController::Reset()
{
    PendingForward = 0.0f;
    PendingRight = 0.0f;
    PendingUp = 0.0f;
    PendingYaw = 0.0f;
    PendingPitch = 0.0f;
    bRequestsCursorHidden = false;
    bRequestsMouseLock = false;
    bAllowsCursorHidden = true;
    bAllowsMouseLock = true;
    ApplyCursorVisibilityState();
    ApplyMouseLockState();
    ControlledPawn = nullptr;
    ControlledCameraComponent = nullptr;
    SyncAnglesFromCamera();
}

void FGameInputController::SetCursorHidden(bool bHidden)
{
    bRequestsCursorHidden = bHidden;
    ApplyCursorVisibilityState();
}

void FGameInputController::SetMouseLocked(bool bLocked)
{
    bRequestsMouseLock = bLocked;
    ApplyMouseLockState();
}

void FGameInputController::SetCursorHiddenAllowed(bool bAllowed)
{
    bAllowsCursorHidden = bAllowed;
    ApplyCursorVisibilityState();
}

void FGameInputController::SetMouseLockAllowed(bool bAllowed)
{
    bAllowsMouseLock = bAllowed;
    ApplyMouseLockState();
}

void FGameInputController::RefreshControlledPawn()
{
    if (World == nullptr || World->GetPersistentLevel() == nullptr)
    {
        ControlledPawn = nullptr;
        ControlledCameraComponent = nullptr;
        SyncAnglesFromCamera();
        return;
    }

    const bool bCurrentPawnInvalid =
        ControlledPawn == nullptr ||
        !UObject::IsValid(ControlledPawn) ||
        ControlledPawn->IsPendingDestroy() ||
        ControlledPawn->GetFocusedWorld() != World;

    if (bCurrentPawnInvalid)
    {
        ControlledPawn = nullptr;
        const TArray<AActor*>& Actors = World->GetPersistentLevel()->GetActors();
        for (AActor* Actor : Actors)
        {
            APawn* Pawn = Cast<APawn>(Actor);
            if (!Pawn || Pawn->IsPendingDestroy())
            {
                continue;
            }

            ControlledPawn = Pawn;
            break;
        }
    }

    ControlledCameraComponent = ControlledPawn ? ControlledPawn->GetCameraComponent() : nullptr;

    if (ControlledCameraComponent)
    {
        const FVector Forward =
            ControlledCameraComponent->GetWorldTransform().GetUnitAxis(EAxis::X).GetSafeNormal();
        Pitch = MathUtil::RadiansToDegrees(std::asin(MathUtil::Clamp(Forward.Z, -1.0f, 1.0f)));
        Yaw = MathUtil::RadiansToDegrees(std::atan2(Forward.Y, Forward.X));
        return;
    }

    SyncAnglesFromCamera();
}

void FGameInputController::SyncViewportCameraFromPawn()
{
    if (!Camera || !ControlledCameraComponent)
    {
        return;
    }

    const FTransform CameraTransform = ControlledCameraComponent->GetWorldTransform();
    const FCameraState& CameraState = ControlledCameraComponent->GetCameraState();

    Camera->SetLocation(CameraTransform.GetLocation());
    Camera->SetRotation(CameraTransform.GetRotation());
    Camera->SetNearPlane(CameraState.NearZ);
    Camera->SetFarPlane(CameraState.FarZ);

    if (CameraState.bIsOrthogonal)
    {
        Camera->SetProjectionType(EViewportProjectionType::Orthographic);
        const float AspectRatio = Camera->GetAspectRatio() > 0.0f ? Camera->GetAspectRatio() : 1.0f;
        Camera->SetOrthoHeight(CameraState.OrthoWidth / AspectRatio);
    }
    else
    {
        Camera->SetProjectionType(EViewportProjectionType::Perspective);
        Camera->SetFOV(CameraState.FOV);
    }
}

void FGameInputController::ApplyCursorVisibilityState()
{
    const bool bShouldHideCursor = bRequestsCursorHidden && bAllowsCursorHidden;
    bCursorHidden = bShouldHideCursor;

    InputSystem& Input = InputSystem::Get();
    Input.SetCursorVisibility(!bShouldHideCursor);
}

void FGameInputController::ApplyMouseLockState()
{
    const bool bShouldLockMouse = bRequestsMouseLock && bAllowsMouseLock;
    bMouseLocked = bShouldLockMouse;

    InputSystem& Input = InputSystem::Get();
    if (!bShouldLockMouse)
    {
        Input.LockMouse(false);
        return;
    }

    if (!Window || ViewportWidth <= 0.0f || ViewportHeight <= 0.0f)
    {
        return;
    }

    POINT Origin = { static_cast<LONG>(ViewportX), static_cast<LONG>(ViewportY) };
    ::ClientToScreen(Window->GetHWND(), &Origin);
    Input.LockMouse(true,
                    static_cast<float>(Origin.x),
                    static_cast<float>(Origin.y),
                    ViewportWidth,
                    ViewportHeight);
}

void FGameInputController::EnsureScriptLoaded()
{
    if (bScriptLoadAttempted)
    {
        return;
    }

    bScriptLoadAttempted = true;
    bScriptLoaded = LoadScript();
}

bool FGameInputController::LoadScript()
{
    if (!GEngine)
    {
        return false;
    }

    sol::state& Lua = GEngine->GetLuaScriptSubsystem().GetLuaState();
    ScriptEnvironment = sol::environment(Lua, sol::create, Lua.globals());
    InstallBindings();

    LoadedScriptPath = ResolveScriptPath();
    sol::load_result LoadedScript = Lua.load_file(LoadedScriptPath);
    if (!LoadedScript.valid())
    {
        sol::error Error = LoadedScript;
        printf("[Lua Input Load Error] %s: %s\n", LoadedScriptPath.c_str(), Error.what());
        return false;
    }

    sol::protected_function ScriptFunction = LoadedScript;
    sol::set_environment(ScriptEnvironment, ScriptFunction);

    sol::protected_function_result Result = ScriptFunction();
    if (!Result.valid())
    {
        sol::error Error = Result;
        printf("[Lua Input Runtime Error] %s: %s\n", LoadedScriptPath.c_str(), Error.what());
        return false;
    }

    return true;
}

void FGameInputController::InstallBindings()
{
    ScriptEnvironment["MoveForward"] = [this](float Value)
    {
        PendingForward += Value;
    };

    ScriptEnvironment["MoveRight"] = [this](float Value)
    {
        PendingRight += Value;
    };

    ScriptEnvironment["MoveUp"] = [this](float Value)
    {
        PendingUp += Value;
    };

    ScriptEnvironment["AddYaw"] = [this](float Value)
    {
        PendingYaw += Value;
    };

    ScriptEnvironment["AddPitch"] = [this](float Value)
    {
        PendingPitch += Value;
    };

    ScriptEnvironment["SetCursorHidden"] = [this](bool bHidden)
    {
        SetCursorHidden(bHidden);
    };

    ScriptEnvironment["IsCursorHidden"] = [this]()
    {
        return bCursorHidden;
    };

    ScriptEnvironment["SetMouseLocked"] = [this](bool bLocked)
    {
        SetMouseLocked(bLocked);
    };

    ScriptEnvironment["IsMouseLocked"] = [this]()
    {
        return bMouseLocked;
    };

    ScriptEnvironment["SetMoveSpeed"] = [this](float Value)
    {
        MoveSpeed = Value;
    };

    ScriptEnvironment["SetLookSensitivity"] = [this](float Value)
    {
        LookSensitivity = Value;
    };

    ScriptEnvironment["GetDeltaTime"] = [this]()
    {
        return CurrentDeltaTime;
    };
}

FString FGameInputController::ResolveScriptPath() const
{
    return FPaths::ToAbsoluteString(FPaths::ToWide(FPaths::Normalize(ScriptPath)));
}

void FGameInputController::ApplyFallbackKeyDown(int32 KeyCode)
{
    switch (KeyCode)
    {
    case 'W':
        PendingForward += 1.0f;
        break;
    case 'S':
        PendingForward -= 1.0f;
        break;
    case 'D':
        PendingRight += 1.0f;
        break;
    case 'A':
        PendingRight -= 1.0f;
        break;
    case 'E':
        PendingUp += 1.0f;
        break;
    case 'Q':
        PendingUp -= 1.0f;
        break;
    default:
        break;
    }
}

void FGameInputController::ApplyFallbackMouseMove(float DeltaX, float DeltaY)
{
    if (!bMouseLocked)
    {
        return;
    }

    PendingYaw += DeltaX;
    PendingPitch -= DeltaY;
}

void FGameInputController::ApplyFallbackMouseClick(const char* ButtonName, bool bPressed)
{
    if (std::string_view(ButtonName) == "RightMouseButton")
    {
        SetCursorHidden(bPressed);
        SetMouseLocked(bPressed);
    }
}

void FGameInputController::ApplyPendingMovement(float DeltaTime)
{
    if (ControlledPawn)
    {
        USceneComponent* Root = ControlledPawn->GetRootComponent();
        UCameraComponent* CameraComponent = ControlledPawn->GetCameraComponent();

        Root->AddWorldOffset(ControlledPawn->GetForwardVector() * (PendingForward * MoveSpeed * DeltaTime));
        Root->AddWorldOffset(ControlledPawn->GetRightVector() * (PendingRight * MoveSpeed * DeltaTime));
        Root->AddWorldOffset(ControlledPawn->GetUpVector() * (PendingUp * MoveSpeed * DeltaTime));
        Root->Rotate(PendingYaw * LookSensitivity, 0.0f);

        if (CameraComponent)
        {
            CameraComponent->AddPitchInput(-PendingPitch * LookSensitivity);
        }

        RefreshControlledPawn();
        return;
    }

    if (!Camera)
    {
        return;
    }

    const FVector Forward = Camera->GetForwardVector().GetSafeNormal();
    const FVector Right = Camera->GetRightVector().GetSafeNormal();
    const FVector Up = FVector::UpVector;
    const FVector Move =
        (Forward * PendingForward + Right * PendingRight + Up * PendingUp) * (MoveSpeed * DeltaTime);

    if (!Move.IsNearlyZero())
    {
        Camera->SetLocation(Camera->GetLocation() + Move);
    }

    Yaw += PendingYaw * LookSensitivity;
    Pitch += PendingPitch * LookSensitivity;
    Pitch = MathUtil::Clamp(Pitch, -89.9f, 89.9f);
    UpdateCameraRotation();
}

void FGameInputController::UpdateCameraRotation()
{
    if (!Camera)
    {
        return;
    }

    const float PitchRadians = MathUtil::DegreesToRadians(Pitch);
    const float YawRadians = MathUtil::DegreesToRadians(Yaw);

    FVector Forward(std::cos(PitchRadians) * std::cos(YawRadians),
                    std::cos(PitchRadians) * std::sin(YawRadians),
                    std::sin(PitchRadians));
    Forward = Forward.GetSafeNormal();

    const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
    const FVector Up = FVector::CrossProduct(Forward, Right).GetSafeNormal();

    FMatrix RotationMatrix = FMatrix::Identity;
    RotationMatrix.SetAxes(Forward, Right, Up);

    FQuat Rotation(RotationMatrix);
    Rotation.Normalize();
    Camera->SetRotation(Rotation);
}

void FGameInputController::SyncAnglesFromCamera()
{
    if (!Camera)
    {
        return;
    }

    const FVector Forward = Camera->GetForwardVector().GetSafeNormal();
    Pitch = MathUtil::RadiansToDegrees(std::asin(MathUtil::Clamp(Forward.Z, -1.0f, 1.0f)));
    Yaw = MathUtil::RadiansToDegrees(std::atan2(Forward.Y, Forward.X));
}

const char* FGameInputController::GetKeyName(int32 KeyCode) const
{
    switch (KeyCode)
    {
    case 'W': return "W";
    case 'A': return "A";
    case 'S': return "S";
    case 'D': return "D";
    case 'Q': return "Q";
    case 'E': return "E";
    default: return "Unknown";
    }
}

const char* FGameInputController::GetMouseButtonName(int32 KeyCode) const
{
    switch (KeyCode)
    {
    case VK_LBUTTON: return "LeftMouseButton";
    case VK_RBUTTON: return "RightMouseButton";
    case VK_MBUTTON: return "MiddleMouseButton";
    default: return "UnknownMouseButton";
    }
}
