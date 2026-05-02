#include "UIManager.h"
#include "Engine/Render/UIBatcher.h"
#include "Engine/Render/FontBatcher.h"
#include "Core/ResourceManager.h"

void FUIManager::Initialize(ID3D11Device* InDevice)
{
    UIBatcher = new FUIBatcher();
    UIBatcher->Create(InDevice);

    FontBatcher = new FFontBatcher();
    FontBatcher->Create(InDevice);
}

void FUIManager::Release()
{
    for (FUIElement* Element : AllElements)
        delete Element;

    AllElements.clear();
    RootElements.clear();

    if (UIBatcher)
    {
        UIBatcher->Release();
        delete UIBatcher;
        UIBatcher = nullptr;
    }

    if (FontBatcher)
    {
        FontBatcher->Release();
        delete FontBatcher;
        FontBatcher = nullptr;
    }
}

void FUIManager::Update(float ViewportW, float ViewportH)
{
    if (!UIBatcher || !FontBatcher) return;

    UIBatcher->Clear();
    FontBatcher->Clear();

    std::sort(RootElements.begin(), RootElements.end(),
        [](const FUIElement* A, const FUIElement* B)
        {
            return A->GetWorldZOrder() < B->GetWorldZOrder();
        });

    for (FUIElement* Root : RootElements)
        RenderRecursive(Root, ViewportW, ViewportH);
}

void FUIManager::Flush(ID3D11DeviceContext* Context, const FRenderBus* RenderBus)
{
    if (!UIBatcher || !FontBatcher) return;

    // 쿼드/이미지 먼저
    UIBatcher->Flush(Context, RenderBus);

    // 텍스트는 항상 위에
    const FFontResource* FontRes = FResourceManager::Get().FindFont(FName("Default"));
    if (FontRes)
        FontBatcher->Flush(Context, RenderBus, FontRes);
}

void FUIManager::RenderRecursive(FUIElement* Element, float ViewportW, float ViewportH)
{
    if (!Element->IsWorldVisible()) return;

    const FVector2 WorldPos = Element->GetWorldPosition();
    const FVector2 Size = Element->Size;

    switch (Element->GetType())
    {
    case EUIElementType::Image:
    {
        auto* Img = static_cast<FUIImage*>(Element);
        UIBatcher->AddQuad(WorldPos, Size, { ViewportW, ViewportH },
            Img->Texture, Img->TintColor);
        break;
    }
    case EUIElementType::ProgressBar:
    {
        auto* Bar = static_cast<FUIProgressBar*>(Element);

        // 배경 쿼드
        UIBatcher->AddQuad(WorldPos, Size, { ViewportW, ViewportH },
            nullptr, Bar->BgColor);

        // 채움 쿼드 — Value(0~1)만큼 가로 크기 축소
        UIBatcher->AddQuad(WorldPos, { Size.X * Bar->GetValue(), Size.Y },
            { ViewportW, ViewportH }, nullptr, Bar->FillColor);
        break;
    }
    case EUIElementType::Text:
    {
        auto* Txt = static_cast<FUIText*>(Element);
        // FontSize를 Scale로 사용 — 기본 폰트 크기(20px) 대비 비율
        const float Scale = Txt->FontSize / 20.f;
        FontBatcher->AddText2D(Txt->Text, WorldPos, ViewportW, ViewportH, Scale);
        break;
    }
    }

    // 자식 재귀 순회
    for (FUIElement* Child : Element->GetChildren())
        RenderRecursive(Child, ViewportW, ViewportH);
}

FUIImage* FUIManager::CreateImage(FUIElement* Parent,
    FVector2 LocalPos, FVector2 Size,
    UTexture* Texture, FVector4 Color)
{
    FUIImage* Element = new FUIImage(LocalPos, Size, Texture, Color);
    RegisterElement(Element, Parent);
    return Element;
}

FUIText* FUIManager::CreateText(FUIElement* Parent,
    FVector2 LocalPos, FVector2 Size,
    const FString& Text, float FontSize, FVector4 Color)
{
    FUIText* Element = new FUIText(LocalPos, Size, Text, FontSize, Color);
    RegisterElement(Element, Parent);
    return Element;
}

FUIProgressBar* FUIManager::CreateProgressBar(FUIElement* Parent,
    FVector2 LocalPos, FVector2 Size,
    FVector4 FillColor, FVector4 BgColor)
{
    FUIProgressBar* Element = new FUIProgressBar(LocalPos, Size, FillColor, BgColor);
    RegisterElement(Element, Parent);
    return Element;
}

void FUIManager::RegisterElement(FUIElement* Element, FUIElement* Parent)
{
    AllElements.push_back(Element);

    if (Parent)
        Parent->AddChild(Element);   // 부모의 Children에 추가
    else
        RootElements.push_back(Element); // 루트 등록
}

// --- 제거 ---

void FUIManager::DestroyElement(FUIElement* Element)
{
    if (!Element) return;

    // 부모의 Children에서 자신을 제거
    if (FUIElement* Parent = Element->GetParent())
        Parent->RemoveChild(Element);
    else
    {
        auto It = std::find(RootElements.begin(), RootElements.end(), Element);
        if (It != RootElements.end())
            RootElements.erase(It);
    }

    DestroyRecursive(Element);
}

void FUIManager::DestroyRecursive(FUIElement* Element)
{
    // 자식부터 재귀 삭제
    for (FUIElement* Child : Element->GetChildren())
        DestroyRecursive(Child);

    auto It = std::find(AllElements.begin(), AllElements.end(), Element);
    if (It != AllElements.end())
        AllElements.erase(It);

    delete Element;
}
