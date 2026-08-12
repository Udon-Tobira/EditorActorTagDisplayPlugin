// Copyright (c) 2026 metyatech. All rights reserved.

#include "ActorMetadataOverlaySettings.h"

#include "GameFramework/Actor.h"

UActorMetadataOverlayProjectSettings::UActorMetadataOverlayProjectSettings()
    : DefaultDisplayTemplate(TEXT("{ActorLabel}\nClass: {ActorClass}\nTags: {ActorTags}"))
{
    FActorMetadataOverlayRule DefaultRule;
    DefaultRule.RuleName = TEXT("Default Actor Metadata");
    DefaultRule.ActorClass = AActor::StaticClass();
    Rules.Add(DefaultRule);
}

UActorMetadataOverlayProjectSettings* UActorMetadataOverlayProjectSettings::Get()
{
    return GetMutableDefault<UActorMetadataOverlayProjectSettings>();
}

#if WITH_EDITOR
void UActorMetadataOverlayProjectSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    SaveConfig();
}
#endif

UActorMetadataOverlayUserSettings* UActorMetadataOverlayUserSettings::Get()
{
    return GetMutableDefault<UActorMetadataOverlayUserSettings>();
}

#if WITH_EDITOR
void UActorMetadataOverlayUserSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    SaveConfig();
}
#endif
