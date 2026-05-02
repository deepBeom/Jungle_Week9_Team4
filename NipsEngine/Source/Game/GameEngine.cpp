#include "Game/GameEngine.h"
#include "Game/GameViewportClient.h"
#include "Game/GameRenderPipeline.h"
#include "Core/Paths.h"
#include "Engine/GameFramework/World.h"
#include "Serialization/SceneSaveManager.h"


// test
#include "Engine/UI/UIManager.h"
#include "Core/ResourceManager.h"


#include <filesystem>

DEFINE_CLASS(UGameEngine, UEngine)

void UGameEngine::Init(FWindowsWindow* InWindow)
{
    UEngine::Init(InWindow);

	assert(LoadStartLevel());

	SetActiveWorld(WorldList[0].ContextHandle);
	WorldList[0].World->SetWorldType(EWorldType::Game);
    ApplySpatialIndexMaintenanceSettings(WorldList[0].World);

	ViewportClient.Initialize(InWindow);
	ViewportClient.SetWorld(WorldList[0].World);

	SetRenderPipeline(std::make_unique<FGameRenderPipeline>(this, Renderer));

    UTexture* IconTex = FResourceManager::Get().GetTexture("Asset/Texture/T_ENV_KNFRST_Block_01_BC.png");
    FUIManager::Get().CreateImage(
        nullptr,                        // 부모 없음 (루트)
        { 400.f, 400.f },               // 스크린 좌표 (픽셀)
        { 200.f, 200.f },              // 크기
        IconTex,                        // 텍스처 (nullptr 이면 흰 박스)
        { 1.f, 0.f, 0.f, 1.f }        // 색상 tint (빨간색)
    );
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
