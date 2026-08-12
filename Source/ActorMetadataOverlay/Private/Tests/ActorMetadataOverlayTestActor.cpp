// Copyright (c) 2026 metyatech. All rights reserved.

#include "ActorMetadataOverlayTestActor.h"

AActorMetadataOverlayTestActor::AActorMetadataOverlayTestActor()
{
    SetFlags(RF_Transient);
    PrimaryActorTick.bCanEverTick = false;
}

void AActorMetadataOverlayTestActor::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
    TagContainer = GameplayTags;
}
