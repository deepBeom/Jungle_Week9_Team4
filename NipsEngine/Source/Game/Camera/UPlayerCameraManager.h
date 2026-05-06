#pragma once
#include "UCameraModifier.h"
#include "Core/Containers/Array.h"
#include <memory>
#include <algorithm>

// Unreal APlayerCameraManager 패턴: CameraModifier 목록을 관리하고
// 매 프레임 각 Modifier를 우선순위 순으로 적용하여 FCameraEffectSettings를 생성한다.
class UPlayerCameraManager
{
public:
    static UPlayerCameraManager& Get();

    void Tick(float DeltaTime);

    // T: UCameraModifier 파생 클래스. 포인터를 반환하여 외부에서 파라미터 설정 가능.
    template<typename T>
    T* AddModifier()
    {
        auto Modifier = std::make_shared<T>();
        T* Raw = Modifier.get();
        Modifiers.push_back(std::move(Modifier));
        SortModifiers();
        return Raw;
    }

    void RemoveModifier(UCameraModifier* Target);
    void ClearModifiers();

    const FCameraEffectSettings& GetCameraEffectSettings() const { return CameraEffects; }

private:
    void SortModifiers();

    TArray<std::shared_ptr<UCameraModifier>> Modifiers;
    FCameraEffectSettings CameraEffects;
};
