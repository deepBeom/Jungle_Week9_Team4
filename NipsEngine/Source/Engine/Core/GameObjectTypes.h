#pragma once

#include "Core/CoreTypes.h"
#include "Runtime/WindowsApplication.h"

enum class EObjectType : int32
{
    None = 0,
    Player,
    Coin,
    Block,
    Box,
    Boss,
    MAX
};

enum EGameplayTagBits : uint32
{
    GT_None = 0,
    GT_Player = 1 << 0,
    GT_Collectible = 1 << 1,
    GT_Enemy = 1 << 2,
    GT_Boss = 1 << 3,
    GT_Damageable = 1 << 4,
    GT_Solid = 1 << 5,
    GT_TriggerOnly = 1 << 6,
    GT_Environment = 1 << 7,
};

enum class EObjectCollsionShape : int32
{
    None = 0,
    Sphere,
    Box,
    Capsule,
};

struct FObjectTypeBinding
{
    EObjectType ObjectType = EObjectType::None;
    
    const char* MeshPath = "";
    const char* LuePath = "";
    
    EObjectCollsionShape CollisionShape = EObjectCollsionShape::None;
    bool bGenerateOverlapEvents = false;
    bool bBlockComponent = false;
    
    uint32 DefaultTags = GT_None;
};

inline const char* ToString(EObjectType Type)
{
    switch (Type)
    {
    case EObjectType::Player: return "Player";
    case EObjectType::Coin:   return "Coin";
    case EObjectType::Block:  return "Block";
    case EObjectType::Box:    return "Box";
    case EObjectType::Boss:   return "Boss";
    case EObjectType::None:
    default:                  return "None";
    }
}

inline const FObjectTypeBinding* FindObjectTypeBinding(EObjectType Type)
{
    static const FObjectTypeBinding Bindings[] = 
    {
        {
            EObjectType::Player,
            "Asset/Mesh/Lumine/LumineModel.obj",
            "Scripts/Player.lua",
            EObjectCollisionShape::Capsule,
            true,
            true,
            GT_Player | GT_Damageable
        },
        {
            EObjectType::Coin,
            "Asset/Mesh/Coin.obj",
            "Scripts/Coin.lua",
            EObjectCollisionShape::Sphere,
            true,
            false,
            GT_Collectible | GT_TriggerOnly
        },
        {
            EObjectType::Block,
            "Asset/Mesh/cube.obj",
            "",
            EObjectCollisionShape::Box,
            false,
            true,
            GT_Solid | GT_Environment
        },
        {
            EObjectType::Box,
            "Asset/Mesh/MultiMaterialCube/MultiMaterialCube.obj",
            "",
            EObjectCollisionShape::Box,
            false,
            true,
            GT_Solid | GT_Environment
        },
        {
            EObjectType::Boss,
            "Asset/Mesh/dragon.obj",
            "Scripts/Boss.lua",
            EObjectCollisionShape::Capsule,
            true,
            true,
            GT_Enemy | GT_Boss | GT_Damageable
        },
    };
    
    for (const FObjectTypeBinding& Binding : Bindings)
    {
        if (Binding.ObjectType == Type)
        {
            return &Binding;
        }
    }

    return nullptr;
}