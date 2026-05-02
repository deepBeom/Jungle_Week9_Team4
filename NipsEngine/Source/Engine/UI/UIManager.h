#pragma once
#include "Engine/Core/Singleton.h"

// 계층적인 UI(FUIElement)와 2D 스페이스 상의 UI(FUIBatcher)를 관리

class FUIBatcher;
class FUIElement;


class FUIManager : public TSingleton<FUIManager>
{
    friend class TSingleton<FUIManager>;
public:
    void Initialize();
    void Release();

    void Tick(float fTimeDelta);

private:
    FUIBatcher* UIBatcher;
    TArray<FUIElement*> UIElements;

};
