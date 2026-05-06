#include "UPlayerCameraManager.h"

UPlayerCameraManager& UPlayerCameraManager::Get()
{
    static UPlayerCameraManager Instance;
    return Instance;
}

void UPlayerCameraManager::Tick(float DeltaTime)
{
    CameraEffects = FCameraEffectSettings{};

    for (auto& Modifier : Modifiers)
    {
        if (Modifier && Modifier->bEnabled)
        {
            Modifier->ModifyCamera(DeltaTime, CameraEffects);
        }
    }

    // IsFinished()가 true인 Modifier는 자동 제거 (FadeIn 완료 등)
    Modifiers.erase(
        std::remove_if(Modifiers.begin(), Modifiers.end(),
            [](const std::shared_ptr<UCameraModifier>& M) { return !M || M->IsFinished(); }),
        Modifiers.end());
}

void UPlayerCameraManager::RemoveModifier(UCameraModifier* Target)
{
    Modifiers.erase(
        std::remove_if(Modifiers.begin(), Modifiers.end(),
            [Target](const std::shared_ptr<UCameraModifier>& M) { return M.get() == Target; }),
        Modifiers.end());
}

void UPlayerCameraManager::ClearModifiers()
{
    Modifiers.clear();
}

void UPlayerCameraManager::SortModifiers()
{
    std::stable_sort(Modifiers.begin(), Modifiers.end(),
        [](const std::shared_ptr<UCameraModifier>& A, const std::shared_ptr<UCameraModifier>& B)
        {
            return A->Priority < B->Priority;
        });
}
