#include "Game/GameEngine.h"
#include "Game/GameViewportClient.h"
#include "Game/GameRenderPipeline.h"
#include "Core/Paths.h"
#include "Engine/GameFramework/World.h"
#include "Serialization/SceneSaveManager.h"

#include <filesystem>

DEFINE_CLASS(UGameEngine, UEngine)

void UGameEngine::Init(FWindowsWindow* InWindow)
{
    UEngine::Init(InWindow);

    if (!LoadStartLevel())
    {
        // TODO: 강제 종료 및 에러 메시지
        return;
    }

	SetActiveWorld(WorldList[0].ContextHandle);
	WorldList[0].World->SetWorldType(EWorldType::Game);
    ApplySpatialIndexMaintenanceSettings(WorldList[0].World);

	ViewportClient.Initialize(InWindow);
	ViewportClient.SetWorld(WorldList[0].World);

	SetRenderPipeline(std::make_unique<FGameRenderPipeline>(this, Renderer));
}

bool UGameEngine::LoadStartLevel()
{
	std::filesystem::path ScenePath = std::filesystem::path(FSceneSaveManager::GetSceneDirectory())
		/ (FPaths::ToWide("Scene_Game") + FSceneSaveManager::SceneExtension);

	FWorldContext LoadedContext;
	FSceneSaveManager::Load(FPaths::ToUtf8(ScenePath.wstring()), LoadedContext);
	if (!LoadedContext.World)
	{
		return false;
	}

	LoadedContext.WorldType = EWorldType::Game;
	LoadedContext.ContextHandle = FName("Game");
	LoadedContext.ContextName = "Game";
	WorldList.push_back(LoadedContext);
	return true;
}

void UGameEngine::Tick(float DeltaTime)
{
	ViewportClient.Tick(DeltaTime);
	UEngine::Tick(DeltaTime);
}

void UGameEngine::OnWindowResized(uint32 Width, uint32 Height)
{
	UEngine::OnWindowResized(Width, Height);
	ViewportClient.SetViewportSize(Window->GetWidth(), Window->GetHeight());
}
