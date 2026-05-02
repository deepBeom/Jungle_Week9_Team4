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


    CreateLogoHud();
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

void UGameEngine::CreateLogoHud()
{
    FUIManager& UI = FUIManager::Get();

    UTexture* LogoTex  = FResourceManager::Get().LoadTexture("Asset/Texture/UI/Logo.png");
    UTexture* Icon1Tex = FResourceManager::Get().LoadTexture("Asset/Texture/UI/Icon_boat.png");
    UTexture* Icon2Tex = FResourceManager::Get().LoadTexture("Asset/Texture/UI/Icon_book.png");

    // ── 루트 패널 (투명, 뷰포트 중앙 기준) ──────────────────────────
    FUIImage* Panel = UI.CreateImage(
        nullptr,
        { 0.5f, 0.5f }, { 0.4f, 0.5f },
        nullptr, { 0, 0, 0, 0 },
        FUICreateParams::FullRelative()
    );

    // ── 로고 (패널 상단, 패널 폭 80% × 높이 30%) ─────────────────────
    FUIImage* Logo = UI.CreateImage(
        Panel,
        { 0.f, -0.3f }, { 0.8f, 0.3f },
        LogoTex, { 1, 1, 1, 1 },
        FUICreateParams::ParentRelative()
    );

    // ── 아이콘 행 1 : Icon1 + 이름 텍스트 + 수치 텍스트 ──────────────
    FUIImage* Icon1 = UI.CreateImage(
        Panel,
        { -0.25f, 0.1f }, { 0.15f, 0.25f },
        Icon1Tex, { 1, 1, 1, 1 },
        FUICreateParams::ParentRelative()
    );

    // 아이콘 오른쪽 레이블 (아이콘 중심보다 0.18 오른쪽, 같은 Y)
    FUIText* Label1 = UI.CreateText(
        Panel,
        { 0.025f, 0.03f }, { 256.f, 28.f },
        "Start",
        48.f,
        { 0.f, 0.f, 0.f, 1.f },
        FUICreateParams::RelativePos()   // 위치만 비율, 크기는 픽셀
    );

    // ── 아이콘 행 2 : Icon2 + 이름 텍스트 + 수치 텍스트 ──────────────
    FUIImage* Icon2 = UI.CreateImage(
        Panel,
        { -0.25f, 0.4f }, { 0.15f, 0.25f },
        Icon2Tex, { 1, 1, 1, 1 },
        FUICreateParams::ParentRelative()
    );

    FUIText* Label2 = UI.CreateText(
        Panel,
        { 0.025f, 0.18f }, { 256.f, 28.f },
        "Record",
        48.f,
        { 0.f, 0.f, 0.f, 1.f },
        FUICreateParams::RelativePos()
    );
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
