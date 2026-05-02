#pragma once
#include "Engine/Input/Controller/ViewportInputController.h"
#include <functional>

class AActor;
class FViewportCamera;
class UWorld;

class FPIEController : public IViewportInputController
{
  public:
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

    void SetCamera(FViewportCamera* InCamera);
    void SetCamera(FViewportCamera& InCamera);
    void NullifyCamera() { Camera = nullptr; }
    void SetWorld(UWorld* InWorld) { World = InWorld; }
    void NullifyWorld() { World = nullptr; }
    void ResetTargetLocation();

    void SetEndPIECallback(std::function<void()> Callback) { OnRequestEndPIE = std::move(Callback); }
    void ClearEndPIECallback() { OnRequestEndPIE = nullptr; }

    FVector GetTargetLocation() const { return TargetLocation; }
    void SetTargetLocation(FVector InTargetLoc) { TargetLocation = InTargetLoc; }

  private:
    void UpdateCameraRotation();
    void PushActorAtScreenPosition(float X, float Y);
    void TickCollectionArea(float DeltaTime);
    void ReleaseCollectionArea();
    AActor* FindBoatActor() const;
    bool IsCollectibleActor(const AActor* Actor) const;
    bool IsActorInsideCollectionRadius(const AActor* Actor, const FVector& Center, float Radius) const;

  private:
    FViewportCamera*      Camera = nullptr;
    UWorld*               World = nullptr;
    std::function<void()> OnRequestEndPIE;

    float                 Yaw = 0.f;
    float                 Pitch = 0.f;
    float                 MoveSpeed = 15.f;
    FVector               TargetLocation;
    bool                  bTargetLocationInitialized = false;
    bool                  bCollectionKeyHeld = false;
    float                 CurrentCollectionRadius = 0.0f;
    float                 CollectionMinRadius = 1.0f;
    float                 CollectionMaxRadius = 10.0f;
    float                 CollectionGrowthRate = 4.0f;
};
