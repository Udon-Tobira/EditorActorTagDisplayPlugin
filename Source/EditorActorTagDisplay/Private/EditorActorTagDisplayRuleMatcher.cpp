// Copyright (c) 2026 metyatech. All rights reserved.

#include "EditorActorTagDisplayRuleMatcher.h"

#include "EditorActorTagDisplayLog.h"
#include "GameFramework/Actor.h"

namespace
{
    void WarnAboutUnresolvedRule(const FActorMetadataOverlayRule& Rule, TSet<FName>& InOutWarnedRuleNames)
    {
        if (!InOutWarnedRuleNames.Contains(Rule.RuleName))
        {
            InOutWarnedRuleNames.Add(Rule.RuleName);
            const FString DisplayName = Rule.RuleName.IsNone() ? TEXT("<unnamed>") : Rule.RuleName.ToString();
            UE_LOG(LogEditorActorTagDisplay, Warning, TEXT("Rule '%s' has an empty or unresolved Actor Class and will be ignored."), *DisplayName);
        }
    }
}

namespace EditorActorTagDisplayRuleMatcher
{
    void ResolveRules(const TArray<FActorMetadataOverlayRule>& Rules,
                      TArray<FResolvedActorMetadataOverlayRule>& OutResolvedRules,
                      TSet<FName>& InOutWarnedRuleNames)
    {
        OutResolvedRules.Reset();
        OutResolvedRules.Reserve(Rules.Num());

        for (const FActorMetadataOverlayRule& Rule : Rules)
        {
            FResolvedActorMetadataOverlayRule ResolvedRule;
            ResolvedRule.Rule = Rule;

            if (Rule.ActorClass.IsNull())
            {
                WarnAboutUnresolvedRule(Rule, InOutWarnedRuleNames);
            }
            else
            {
                ResolvedRule.ResolvedActorClass = Rule.ActorClass.LoadSynchronous();
                if (ResolvedRule.ResolvedActorClass == nullptr)
                {
                    WarnAboutUnresolvedRule(Rule, InOutWarnedRuleNames);
                }
            }

            OutResolvedRules.Add(MoveTemp(ResolvedRule));
        }
    }

    int32 FindMatchingRule(const AActor& Actor, const TArray<FResolvedActorMetadataOverlayRule>& Rules)
    {
        for (int32 RuleIndex = 0; RuleIndex < Rules.Num(); ++RuleIndex)
        {
            const FResolvedActorMetadataOverlayRule& ResolvedRule = Rules[RuleIndex];
            const FActorMetadataOverlayRule& Rule = ResolvedRule.Rule;

            if (!Rule.bEnabled || ResolvedRule.ResolvedActorClass == nullptr)
            {
                continue;
            }

            const bool bClassMatches = Rule.bIncludeDerivedClasses
                                            ? Actor.IsA(ResolvedRule.ResolvedActorClass)
                                            : Actor.GetClass() == ResolvedRule.ResolvedActorClass;
            if (!bClassMatches)
            {
                continue;
            }

            bool bRequiredTagsMatch = true;
            for (const FName& RequiredTag : Rule.RequiredActorTags)
            {
                if (!Actor.Tags.Contains(RequiredTag))
                {
                    bRequiredTagsMatch = false;
                    break;
                }
            }
            if (!bRequiredTagsMatch)
            {
                continue;
            }

            bool bExcludedTagFound = false;
            for (const FName& ExcludedTag : Rule.ExcludedActorTags)
            {
                if (Actor.Tags.Contains(ExcludedTag))
                {
                    bExcludedTagFound = true;
                    break;
                }
            }
            if (bExcludedTagFound)
            {
                continue;
            }

            return RuleIndex;
        }

        return INDEX_NONE;
    }
}
