#pragma once

#include "Engine/Input/Controller/ViewportInputController.h"
#include "Math/Vector.h"

class FViewportCamera;

class FGameCameraController : public IViewportInputController
{
public:
    void SetCamera(FViewportCamera* InCamera);

    void Tick(float InDeltaTime) override;
    void OnMouseMove(float DeltaX, float DeltaY) override;
    void OnLeftMouseClick(float X, float Y) override;
    void OnLeftMouseDragEnd(float X, float Y) override;
    void OnLeftMouseButtonUp(float X, float Y) override;
    void OnRightMouseClick(float DeltaX, float DeltaY) override;
    void OnLeftMouseDrag(float X, float Y) override;
    void OnRightMouseDrag(float DeltaX, float DeltaY) override;
    void OnMiddleMouseDrag(float DeltaX, float DeltaY) override;
    void OnKeyPressed(int VK) override;
    void OnKeyDown(int VK) override;
    void OnKeyReleased(int VK) override;
    void OnWheelScrolled(float Notch) override;

private:
    void UpdateCameraRotation(float InDeltaTime);

private:
    FViewportCamera* Camera = nullptr;

    float Yaw = 0.f;
    float Pitch = 0.f;
    float LookDeltaX = 0.f;
    float LookDeltaY = 0.f;
};
