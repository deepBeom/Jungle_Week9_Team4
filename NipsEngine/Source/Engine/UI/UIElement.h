#pragma once
#include "Engine/Render/Resource/Texture.h"

enum class EUIElementType : uint32
{
    UI_Image,
    UI_Text,
    UI_ProgressBar,
};

class FUIElemet
{
    FVector2 Position;
    FVector2 Size;
    float    OrderZ;
    bool     bIsVisible = true;

    virtual EUIElementType GetType() = 0;
};


class FUIIMage : public FUIElemet
{
    // FUIElemet을(를) 통해 상속됨
    EUIElementType GetType() override { return EUIElementType::UI_Image;  };
    UTexture* Texture = { nullptr };
    FVector4  TintColor = { 1.f, 1.f, 1.f, 1.f };
};

class FUIText : public FUIElemet
{
    // FUIElemet을(를) 통해 상속됨
    EUIElementType GetType() override { return EUIElementType::UI_Text; };

    float FontSize = { 16.f };
    FVector4 Color = { 1.f, 1.f, 1.f,1.f };
};

class FUIProgressBar : public FUIElemet
{
    EUIElementType GetType() override { return EUIElementType::UI_ProgressBar; };

    float Value = { 1.f };
    FVector4 FillColor = { 0.8f, 0.8f, 0.8f, 1.f };
    FVector4 BGColor = { 0.2f, 0.2f, 0.2f, 1.f };
};



