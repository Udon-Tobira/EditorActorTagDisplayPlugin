// Copyright (c) 2026 metyatech. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;

class FActorMetadataOverlayTemplateFormatter
{
public:
    static FString Format(const AActor& Actor,
                          const FString& Template,
                          int32 MaxPropertyValueLength,
                          TSet<FString>& InOutWarnedPropertyKeys);

private:
    static FString FormatProperty(const AActor& Actor,
                                  const FString& PropertyName,
                                  int32 MaxPropertyValueLength,
                                  TSet<FString>& InOutWarnedPropertyKeys);
};
