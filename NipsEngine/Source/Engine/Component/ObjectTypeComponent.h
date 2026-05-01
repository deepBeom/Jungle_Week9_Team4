#pragma once
#include <d3d11.h>

#include "ActorComponent.h"
#include "Core/GameObjectTypes.h"

class UObjectTypeComponent : public UActorComponent
{
public:
    DECLARE_CLASS(UObjectTypeComponent, UActorComponent);
    
    UObjectTypeComponent() = default;
    
    void Serialize(FArchive& Ar) override;
    void GetEditableProperties(TArray<FPropertyDescriptor>& OutProps) override;
    void PostEditProperty(const char* PropertyName) override;
    
    EObjectType GetObjectType() const
    {
        return static_cast<EObjectType>(ObjectTypeValue);
    }
    
    void SetObjectType(EObjectType NewType)
    {
        ObjectTypeValue = static_cast<int32>(NewType);
        ApplyDefaultTagsForObjectType();
    }
    
    uint32 GetGameplayTagMask() const
    {
        return GameplayTagMask;
    }
    
    void SetGameplayTagMask(uint32 NewMask)
    {
        GameplayTagMask = NewMask;
    }
    
    bool HasTag(EGameplayTagBits Tag) const
    {
        return (GameplayTagMask & static_cast<uint32>(Tag)) != 0;
    }
    
    void AddTag(EGameplayTagBits Tag)
    {
        GameplayTagMask |= static_cast<uint32>(Tag);
    }
    
    void RemoveTag(EGameplayTagBits Tag)
    {
        GameplayTagMask &= ~static_cast<uint32>(Tag);
    }
    
private:
    void ApplyDefaultsForObjectType();
    void ApplyDefaultTagsForObjectType();
    void ApplyDefaultMeshForObjectType();
    
private:
    int32 ObjectTypeValue = static_cast<int32>(EObjectType::None);
    uint32 GameplayTagMask = GT_None;
    
    bool bAutoApplyDefaultTags = true;
    bool bAutoAppluObjectDefaults = true;
};
