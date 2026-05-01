#pragma once

#include "Engine/Component/ActorComponent.h"

struct FHitResult;
struct FLuaScriptInstance;

class UScriptComponent : public UActorComponent
{
public:
	DECLARE_CLASS(UScriptComponent, UActorComponent)

    void BeginPlay() override;
    void EndPlay() override;
    void TickComponent(float DeltaTime) override;
    void Activate() override;
    void Deactivate() override;
    void Serialize(FArchive& Ar) override;
    void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
    void PostEditProperty(const char* PropertyName) override;

    void OnOverlap(AActor* OtherActor);
    void OnOverlapBegin(AActor* OtherActor);
    void OnOverlapEnd(AActor* OtherActor);
    void OnHit(AActor* OtherActor);
    void OnHit(AActor* OtherActor, const FHitResult& HitInfo);

    bool ReloadScript();

    void SetScriptPath(const std::string& InPath);
    const FString& GetScriptPath() const { return ScriptPath; }
    bool IsScriptLoaded() const;

private:
    bool EnsureScriptInstance();
    void StartScriptIfNeeded();
    void NotifyScriptEnabled();
    void NotifyScriptDisabled();
    void NotifyScriptDestroyed();
    void DisableScriptAfterFatalError();
    void HandleRuntimeCallbackResult(bool bSucceeded, bool bHadCallback);
    bool TryCallPreferred(const std::initializer_list<const char*>& CallbackNames);
    bool TryCallPreferredFloat(const std::initializer_list<const char*>& CallbackNames, float Value);
    bool TryCallPreferredActor(const std::initializer_list<const char*>& CallbackNames, AActor* OtherActor);
    bool TryCallPreferredHit(const std::initializer_list<const char*>& CallbackNames, AActor* OtherActor, const FHitResult& HitInfo);

private:
    FString ScriptPath;
    bool bSerializedEnabled = true;
    bool bHasBegunPlay = false;
    bool bStarted = false;
    bool bEnableNotified = false;
    bool bDestroyNotified = false;
    int32 ConsecutiveRuntimeErrorCount = 0;
    std::shared_ptr<FLuaScriptInstance> ScriptInstance;
};
