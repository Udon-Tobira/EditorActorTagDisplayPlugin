// Copyright (c) 2026 metyatech. All rights reserved.

#include "EditorActorTagDisplaySettings.h"

#include "GameFramework/Actor.h"

UEditorActorTagDisplayProjectSettings::UEditorActorTagDisplayProjectSettings()
    : DefaultDisplayTemplate(TEXT("{ActorLabel}\nClass: {ActorClass}\nTags: {ActorTags}"))
{
    FActorMetadataOverlayRule DefaultRule;
    DefaultRule.RuleName = TEXT("Default Actor Metadata");
    DefaultRule.ActorClass = AActor::StaticClass();
    Rules.Add(DefaultRule);
}

UEditorActorTagDisplayProjectSettings* UEditorActorTagDisplayProjectSettings::Get()
{
    return GetMutableDefault<UEditorActorTagDisplayProjectSettings>();
}

#if WITH_EDITOR
void UEditorActorTagDisplayProjectSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    SaveConfig();
}
#endif

UEditorActorTagDisplayUserSettings* UEditorActorTagDisplayUserSettings::Get()
{
    return GetMutableDefault<UEditorActorTagDisplayUserSettings>();
}

#if WITH_EDITOR
void UEditorActorTagDisplayUserSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    SaveConfig();
}
#endif
