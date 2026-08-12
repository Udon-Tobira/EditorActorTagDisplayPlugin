// Copyright (c) 2026 metyatech. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActorMetadataOverlayTypes.h"

class AActor;
class UClass;

struct FResolvedActorMetadataOverlayRule
{
    FActorMetadataOverlayRule Rule;
    UClass* ResolvedActorClass = nullptr;
};

namespace ActorMetadataOverlayRuleMatcher
{
    void ResolveRules(const TArray<FActorMetadataOverlayRule>& Rules,
                      TArray<FResolvedActorMetadataOverlayRule>& OutResolvedRules,
                      TSet<FName>& InOutWarnedRuleNames);

    int32 FindMatchingRule(const AActor& Actor, const TArray<FResolvedActorMetadataOverlayRule>& Rules);
}
