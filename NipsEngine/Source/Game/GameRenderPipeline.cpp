#include "GameRenderPipeline.h"

#include "Game/GameEngine.h"
#include "Game/GameViewportClient.h"
#include "Engine/GameFramework/World.h"
#include "Engine/Runtime/SceneView.h"
#include "Engine/Render/Renderer/Renderer.h"

FGameRenderPipeline::FGameRenderPipeline(UGameEngine* InGameEngine, FRenderer& InRenderer) : GameEngine(InGameEngine)
{
	Collector.Initialize(InRenderer.GetFD3DDevice().GetDevice());
}

FGameRenderPipeline::~FGameRenderPipeline()
{
    Collector.Release();
}

void FGameRenderPipeline::Execute(float DeltaTime, FRenderer& Renderer)
{
    (void)DeltaTime;

    if (!GameEngine || !GameEngine->GetWorld())
    {
        return;
    }

    Renderer.BeginFrame();
    RenderViewport(Renderer);
    Renderer.UseBackBufferRenderTargets();
    Renderer.EndFrame();
}

void FGameRenderPipeline::RenderViewport(FRenderer& Renderer)
{
	FSceneView SceneView;
    FGameViewportClient* ViewportClient = nullptr;
    if (!PrepareViewport(Renderer, SceneView, ViewportClient))
    {
        return;
    }

    UWorld* World = ViewportClient->GetWorld();
    const FShowFlags ShowFlags;
    const EViewMode ViewMode = SceneView.ViewMode;
    const FFrustum& ViewFrustum = SceneView.CameraFrustum;

    Collector.CollectWorld(World, ShowFlags, ViewMode, Bus, &ViewFrustum);

    Renderer.PrepareBatchers(Bus);
    Renderer.Render(Bus);
}

bool FGameRenderPipeline::PrepareViewport(FRenderer& Renderer, FSceneView& OutSceneView, FGameViewportClient*& OutViewportClient)
{
    (void)Renderer;

    OutViewportClient = &GameEngine->GetViewportClient();
    if (OutViewportClient == nullptr || OutViewportClient->GetWorld() == nullptr)
    {
        return false;
    }

    OutViewportClient->BuildSceneView(OutSceneView);

    const FViewportRect& Rect = OutSceneView.ViewRect;
    if (Rect.Width <= 0 || Rect.Height <= 0)
    {
        return false;
    }

    Bus.Clear();
    Bus.SetViewProjection(OutSceneView.ViewMatrix, OutSceneView.ProjectionMatrix);
    Bus.SetCameraPlane(OutSceneView.NearPlane, OutSceneView.FarPlane);
    Bus.SetRenderSettings(OutSceneView.ViewMode, FShowFlags());
    Bus.SetViewportSize(FVector2(static_cast<float>(Rect.Width), static_cast<float>(Rect.Height)));
    Bus.SetViewportOrigin(FVector2(static_cast<float>(Rect.X), static_cast<float>(Rect.Y)));
    Bus.SetFXAAEnabled(!OutSceneView.bOrthographic);
    Bus.SetShadowFilterType(EShadowFilterType::PCF);

    return true;
}
