#pragma once

#include "Core/CoreTypes.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Math/Vector2.h"
#include "Engine/Render/Resource/Texture.h"

enum class EUIElementType : uint32
{
    Image,
    Text,
    ProgressBar,
};

// FUIElement — 순수 데이터, 렌더링 코드 없음
// LocalPosition은 부모 기준 상대 좌표, GetWorldPosition()으로 최종 스크린 좌표를 얻음
class FUIElement
{
public:
    explicit FUIElement(FVector2 InLocalPos, FVector2 InSize)
        : LocalPosition(InLocalPos), Size(InSize) {}

    virtual ~FUIElement() = default;

    virtual EUIElementType GetType() const = 0;

    // 부모 체인을 타고 올라가며 스크린 좌표 합산
    FVector2 GetWorldPosition() const
    {
        if (Parent)
            return Parent->GetWorldPosition() + LocalPosition;
        return LocalPosition;
    }

    // 부모가 숨겨지면 자식도 숨겨짐
    bool IsWorldVisible() const
    {
        if (!bVisible) return false;
        if (Parent)    return Parent->IsWorldVisible();
        return true;
    }

    // 부모 ZOrder에 로컬 ZOrder 합산 (값 클수록 위에 그려짐)
    float GetWorldZOrder() const
    {
        if (Parent)
            return Parent->GetWorldZOrder() + LocalZOrder;
        return LocalZOrder;
    }

    void        SetVisible(bool bInVisible) { bVisible = bInVisible; }
    bool        IsVisible()          const  { return bVisible; }
    FUIElement* GetParent()          const  { return Parent; }

    const TArray<FUIElement*>& GetChildren() const { return Children; }

    void AddChild(FUIElement* Child)
    {
        Child->Parent = this;
        Children.push_back(Child);
    }

    void RemoveChild(FUIElement* Child)
    {
        auto It = std::find(Children.begin(), Children.end(), Child);
        if (It != Children.end())
        {
            (*It)->Parent = nullptr;
            Children.erase(It);
        }
    }

public:
    FVector2 LocalPosition;   // 부모 기준 상대 좌표
    FVector2 Size;
    float    LocalZOrder = 0.f;

private:
    bool                bVisible  = true;
    FUIElement*         Parent    = nullptr;
    TArray<FUIElement*> Children;
};


class FUIImage : public FUIElement
{
public:
    FUIImage(FVector2 InPos, FVector2 InSize, UTexture* InTexture, FVector4 InColor)
        : FUIElement(InPos, InSize), Texture(InTexture), TintColor(InColor) {}

    EUIElementType GetType() const override { return EUIElementType::Image; }

public:
    UTexture* Texture   = nullptr;
    FVector4  TintColor = { 1.f, 1.f, 1.f, 1.f };
};


class FUIText : public FUIElement
{
public:
    FUIText(FVector2 InPos, FVector2 InSize, const FString& InText, float InFontSize, FVector4 InColor)
        : FUIElement(InPos, InSize), Text(InText), FontSize(InFontSize), Color(InColor) {}

    EUIElementType GetType() const override { return EUIElementType::Text; }

public:
    FString  Text;
    float    FontSize = 16.f;
    FVector4 Color    = { 1.f, 1.f, 1.f, 1.f };
};


class FUIProgressBar : public FUIElement
{
public:
    FUIProgressBar(FVector2 InPos, FVector2 InSize, FVector4 InFillColor, FVector4 InBgColor)
        : FUIElement(InPos, InSize), FillColor(InFillColor), BgColor(InBgColor) {}

    EUIElementType GetType() const override { return EUIElementType::ProgressBar; }

    void  SetValue(float InValue) { Value = InValue < 0.f ? 0.f : (InValue > 1.f ? 1.f : InValue); }
    float GetValue()        const { return Value; }

public:
    FVector4 FillColor = { 0.8f, 0.1f, 0.1f, 1.f };
    FVector4 BgColor   = { 0.2f, 0.2f, 0.2f, 1.f };

private:
    float Value = 1.f;
};
