// Copyright (c) 2026 metyatech. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ActorMetadataOverlayTypes.h"
#include "ActorMetadataOverlaySettings.generated.h"

UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Actor Metadata Overlay"))
class ACTORMETADATAOVERLAY_API UActorMetadataOverlayProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UActorMetadataOverlayProjectSettings();

    virtual FName GetContainerName() const override { return TEXT("Project"); }
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
    virtual FName GetSectionName() const override { return TEXT("ActorMetadataOverlay"); }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    static UActorMetadataOverlayProjectSettings* Get();

    UPROPERTY(config, EditAnywhere, Category = "Actor Metadata Overlay")
    TArray<FActorMetadataOverlayRule> Rules;

    UPROPERTY(config, EditAnywhere, Category = "Actor Metadata Overlay", meta = (MultiLine = "true"))
    FString DefaultDisplayTemplate;

    UPROPERTY(config, EditAnywhere, Category = "Actor Metadata Overlay")
    FLinearColor OutlineColor = FLinearColor::Black;

    UPROPERTY(config, EditAnywhere, Category = "Actor Metadata Overlay", meta = (ClampMin = "16", ClampMax = "1024", UIMin = "16", UIMax = "1024"))
    int32 MaxPropertyValueLength = 120;
};

UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Actor Metadata Overlay"))
class ACTORMETADATAOVERLAY_API UActorMetadataOverlayUserSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UActorMetadataOverlayUserSettings() = default;

    virtual FName GetContainerName() const override { return TEXT("Editor"); }
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
    virtual FName GetSectionName() const override { return TEXT("ActorMetadataOverlay"); }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    static UActorMetadataOverlayUserSettings* Get();

    UPROPERTY(config, EditAnywhere, Category = "Actor Metadata Overlay")
    EActorMetadataOverlayMode DisplayMode = EActorMetadataOverlayMode::Selected;

    UPROPERTY(config, EditAnywhere, Category = "Actor Metadata Overlay", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float GlobalMaxDrawDistance = 10000.0f;

    UPROPERTY(config, EditAnywhere, Category = "Actor Metadata Overlay", meta = (ClampMin = "0.25", ClampMax = "4.0", UIMin = "0.25", UIMax = "4.0"))
    float TextScale = 1.0f;

    UPROPERTY(config, EditAnywhere, Category = "Actor Metadata Overlay")
    bool bOutlined = true;

    UPROPERTY(config, EditAnywhere, Category = "Actor Metadata Overlay")
    bool bDrawBoundingBoxes = true;
};
