#pragma once

#include "Core/CoreTypes.h"
#include "Core/Containers/Array.h"
#include "Core/Singleton.h"
#include "Math/Vector2.h"
#include "Engine/UI/UIElement.h"

class FUIBatcher;
class FFontBatcher;
struct ID3D11Device;
struct ID3D11DeviceContext;
class FRenderBus;

// FUIManager — 계층적 UIElement 트리를 소유하고 매 프레임 UIBatcher로 렌더링
class FUIManager : public TSingleton<FUIManager>
{
    friend class TSingleton<FUIManager>;

public:
    void Initialize(ID3D11Device* InDevice);
    void Release();

    // 게임 루프에서 호출 — 배처 초기화 후 Element 트리를 순회해 버텍스 데이터 누적
    void Update(float ViewportW, float ViewportH);

    // 렌더 패스에서 호출 — 누적된 버텍스를 GPU에 올리고 DrawCall
    void Flush(ID3D11DeviceContext* Context, const FRenderBus* RenderBus);

    // --- Element 생성 (parent가 nullptr이면 루트로 등록) ---
    FUIImage* CreateImage(FUIElement* Parent,
        FVector2 LocalPos, FVector2 Size,
        UTexture* Texture,
        FVector4 Color = { 1, 1, 1, 1 });

    FUIText* CreateText(FUIElement* Parent,
        FVector2 LocalPos, FVector2 Size,
        const FString& Text,
        float FontSize = 16.f,
        FVector4 Color = { 1, 1, 1, 1 });

    FUIProgressBar* CreateProgressBar(FUIElement* Parent,
        FVector2 LocalPos, FVector2 Size,
        FVector4 FillColor = { 0.8f, 0.1f, 0.1f, 1.f },
        FVector4 BgColor = { 0.2f, 0.2f, 0.2f, 1.f });

    // Element와 그 자식 전체를 트리에서 제거하고 메모리 해제
    void DestroyElement(FUIElement* Element);

private:
    // Element를 UIBatcher에 제출하고 자식을 재귀 순회
    void RenderRecursive(FUIElement* Element, float ViewportW, float ViewportH);

    // Element가 루트인지 판별해 RootElements에 추가
    void RegisterElement(FUIElement* Element, FUIElement* Parent);

    // RootElements에서 제거 (자식 정리는 DestroyRecursive가 처리)
    void DestroyRecursive(FUIElement* Element);

private:
    FUIBatcher*         UIBatcher   = nullptr;
    FFontBatcher*       FontBatcher = nullptr;
    TArray<FUIElement*> RootElements;   // 부모 없는 최상위 노드만 보관
    TArray<FUIElement*> AllElements;    // 소유권 — 전체 Element 목록
};
