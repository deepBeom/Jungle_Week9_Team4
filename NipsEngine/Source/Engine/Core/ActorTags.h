#pragma once

#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

namespace ActorTags
{
    inline constexpr const char* Untagged = "Untagged";

    inline const TArray<FString>& GetBuiltinTags()
    {
        static const TArray<FString> Tags =
        {
            Untagged,
            "Respawn",
            "Finish",
            "EditorOnly",
            "MainCamera",
            "Player",
            "GameController",
            "Coin",
            "Boss",
            "Enemy",
            "Collectible",
            "Environment",
        };

        return Tags;
    }

    inline FString Normalize(const FString& Tag)
    {
        return Tag.empty() ? FString(Untagged) : Tag;
    }
}
