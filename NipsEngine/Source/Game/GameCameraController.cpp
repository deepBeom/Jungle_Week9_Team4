#include "Game/GameCameraController.h"
#include "Editor/Viewport/ViewportCamera.h"

#include <cmath>

void FGameCameraController::SetCamera(FViewportCamera* InCamera)
{
    Camera = InCamera;
    FVector Forward = Camera->GetForwardVector();
    Pitch = MathUtil::RadiansToDegrees(std::asin(MathUtil::Clamp(Forward.Z, -1.f, 1.f)));
    Yaw = MathUtil::RadiansToDegrees(std::atan2(Forward.Y, Forward.X));
}

void FGameCameraController::Tick(float InDeltaTime)
{
    IViewportInputController::Tick(InDeltaTime);
    UpdateCameraRotation(InDeltaTime);
    LookDeltaX = 0.f;
    LookDeltaY = 0.f;
}

void FGameCameraController::OnMouseMove(float DeltaX, float DeltaY)
{
    LookDeltaX += DeltaX;
    LookDeltaY += DeltaY;
}

void FGameCameraController::OnLeftMouseClick(float X, float Y)
{
}

void FGameCameraController::OnLeftMouseDragEnd(float X, float Y)
{
}

void FGameCameraController::OnLeftMouseButtonUp(float X, float Y)
{
}

void FGameCameraController::OnRightMouseClick(float DeltaX, float DeltaY)
{
}

void FGameCameraController::OnLeftMouseDrag(float X, float Y)
{
}

void FGameCameraController::OnRightMouseDrag(float DeltaX, float DeltaY)
{
}

void FGameCameraController::OnMiddleMouseDrag(float DeltaX, float DeltaY)
{
}

void FGameCameraController::OnKeyPressed(int VK)
{
}

void FGameCameraController::OnKeyDown(int VK)
{
}

void FGameCameraController::OnKeyReleased(int VK)
{
}

void FGameCameraController::OnWheelScrolled(float Notch)
{
}

void FGameCameraController::UpdateCameraRotation(float InDeltaTime)
{
    constexpr float LookSensitivity = 0.15f;

    Yaw += LookDeltaX * LookSensitivity;
    Pitch -= LookDeltaY * LookSensitivity;
    Pitch = MathUtil::Clamp(Pitch, -89.f, 89.f);

    const float PitchRad = MathUtil::DegreesToRadians(Pitch);
    const float YawRad = MathUtil::DegreesToRadians(Yaw);

    FVector Forward(std::cos(PitchRad) * std::cos(YawRad), std::cos(PitchRad) * std::sin(YawRad), std::sin(PitchRad));
    Forward = Forward.GetSafeNormal();

    const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
    const FVector Up = FVector::CrossProduct(Forward, Right).GetSafeNormal();

    FMatrix RotMat = FMatrix::Identity;
    RotMat.SetAxes(Forward, Right, Up);

    FQuat NewRotation(RotMat);
    NewRotation.Normalize();
    Camera->SetRotation(NewRotation);
}
