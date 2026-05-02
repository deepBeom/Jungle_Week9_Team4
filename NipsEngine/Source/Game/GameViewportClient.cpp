#include "Game/GameViewportClient.h"
#include "Engine/GameFramework/World.h"
#include "Engine/Runtime/SceneView.h"
#include "Engine/Input/InputSystem.h"

void FGameViewportClient::Initialize(FWindowsWindow* InWindow)
{
	FViewportClient::Initialize(InWindow);
	Camera = FViewportCamera();
    CameraController.SetCamera(&Camera);
    CameraController.SetViewportDim(0.f, 0.f, WindowWidth, WindowHeight);
    Camera.OnResize(static_cast<uint32>(WindowWidth), static_cast<uint32>(WindowHeight));
}

void FGameViewportClient::SetViewportSize(float InWidth, float InHeight)
{
    FViewportClient::SetViewportSize(InWidth, InHeight);
    Camera.OnResize(static_cast<uint32>(WindowWidth), static_cast<uint32>(WindowHeight));
    CameraController.SetViewportDim(0.f, 0.f, WindowWidth, WindowHeight);
}

void FGameViewportClient::Tick(float DeltaTime)
{
	TickInput(DeltaTime);
}

void FGameViewportClient::BuildSceneView(FSceneView& OutView) const
{
    OutView.ViewMatrix = Camera.GetViewMatrix();
    OutView.ProjectionMatrix = Camera.GetProjectionMatrix();
    OutView.ViewProjectionMatrix = OutView.ViewMatrix * OutView.ProjectionMatrix;
    OutView.CameraPosition = Camera.GetLocation();
    OutView.CameraForward = Camera.GetForwardVector();
    OutView.CameraRight = Camera.GetRightVector();
    OutView.CameraUp = Camera.GetUpVector();
    OutView.NearPlane = Camera.GetNearPlane();
    OutView.FarPlane = Camera.GetFarPlane();
    OutView.bOrthographic = Camera.IsOrthographic();
    OutView.CameraOrthoHeight = Camera.GetOrthoHeight();
    OutView.CameraFrustum = Camera.GetFrustum();

    OutView.ViewRect = FViewportRect(0, 0, static_cast<int32>(WindowWidth), static_cast<int32>(WindowHeight));
    OutView.ViewMode = EViewMode::Lit;
}

void FGameViewportClient::TickInput(float DeltaTime)
{
    InputSystem& IS = InputSystem::Get();

    constexpr int MoveKeys[] = {'W', 'S', 'A', 'D', 'Q', 'E'};
    for (int VK : MoveKeys)
    {
        if (IS.GetKeyDown(VK))
        {
            CameraController.OnKeyPressed(VK);
        }
        if (IS.GetKey(VK))
        {
            CameraController.OnKeyDown(VK);
        }
        if (IS.GetKeyUp(VK))
        {
            CameraController.OnKeyReleased(VK);
        }
    }

    if (IS.MouseMoved())
    {
        CameraController.OnMouseMove(static_cast<float>(IS.MouseDeltaX()), static_cast<float>(IS.MouseDeltaY()));
    }

    CameraController.Tick(DeltaTime);
}
