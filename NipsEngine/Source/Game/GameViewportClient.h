#pragma once

#include "Engine/Runtime/ViewportClient.h"
#include "Game/GameCameraController.h"
// TODO: 공통로직이므로 Editor에서 Engine으로 옮겨야 함
#include "Editor/Viewport/ViewportCamera.h"

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

	// TODO: Input 처리 필요..
	void TickInput(float DeltaTime);

private:
    UWorld* World = nullptr;
    // TODO: Camera의 직접 생성 대신 Player/CameraComponent에서 가져오는게 맞을 듯
    FViewportCamera Camera;
    FGameCameraController CameraController;

    // TODO: GameInstance, AudioDivice
};
