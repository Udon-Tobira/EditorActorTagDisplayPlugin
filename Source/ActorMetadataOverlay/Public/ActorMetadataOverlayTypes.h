// Copyright (c) 2026 metyatech. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "ActorMetadataOverlayTypes.generated.h"

class AActor;

UENUM()
enum class EActorMetadataOverlayMode : uint8
{
    Off UMETA(DisplayName = "Off"),
    Selected UMETA(DisplayName = "Selected Actors"),
    All UMETA(DisplayName = "All Matching Actors")
};

USTRUCT()
struct ACTORMETADATAOVERLAY_API FActorMetadataOverlayRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    FName RuleName = TEXT("New Rule");

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    TSoftClassPtr<AActor> ActorClass;

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    bool bIncludeDerivedClasses = true;

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    TArray<FName> RequiredActorTags;

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    TArray<FName> ExcludedActorTags;

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    FString DisplayTemplate;

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    FLinearColor DisplayColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    FVector WorldOffset = FVector(0.0f, 0.0f, 20.0f);

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float MaxDrawDistance = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    bool bSelectedOnly = false;

    UPROPERTY(EditAnywhere, Category = "Actor Metadata Overlay")
    bool bDrawBoundingBox = false;
};
