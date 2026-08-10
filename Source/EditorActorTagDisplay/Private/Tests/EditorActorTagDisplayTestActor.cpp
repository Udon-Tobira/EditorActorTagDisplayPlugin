// Copyright (c) 2026 metyatech. All rights reserved.

#include "EditorActorTagDisplayTestActor.h"

AEditorActorTagDisplayTestActor::AEditorActorTagDisplayTestActor()
{
    SetFlags(RF_Transient);
    PrimaryActorTick.bCanEverTick = false;
}

void AEditorActorTagDisplayTestActor::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
    TagContainer = GameplayTags;
}
