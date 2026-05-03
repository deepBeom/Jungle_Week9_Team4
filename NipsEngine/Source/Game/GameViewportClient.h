#pragma once

#include "Engine/Runtime/ViewportClient.h"
#include "Game/GameCameraController.h"
#include "Engine/Viewport/ViewportCamera.h"

class UWorld;
// StandAlone(Shipping) 빌드 전용
class FGameViewportClient : public FViewportClient
{
public:
    FGameViewportClient() = default;
    virtual ~FGameViewportClient() override = default;

    virtual void Initialize(FWindowsWindow* InWindow) override;
    virtual void SetViewportSize(float InWidth, float InHeight) override;
    virtual void Tick(float DeltaTime) override;
    virtual void BuildSceneView(FSceneView& OutView) const override;

    void SetWorld(UWorld* InWorld) { World = InWorld; }
    UWorld* GetWorld() const { return World; }

    FViewportCamera& GetCamera() { return Camera; }

	void TickInput(float DeltaTime);

private:
    UWorld* World = nullptr;
    FViewportCamera Camera;
    FGameCameraController CameraController;

    // TODO: GameInstance, AudioDivice
};
