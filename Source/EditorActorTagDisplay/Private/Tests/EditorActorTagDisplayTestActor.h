// Copyright (c) 2026 metyatech. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "EditorActorTagDisplayTestActor.generated.h"

UENUM()
enum class EEditorActorTagDisplayTestEnum : uint8
{
    First,
    Second
};

UCLASS(Transient, NotPlaceable)
class AEditorActorTagDisplayTestActor : public AActor, public IGameplayTagAssetInterface
{
    GENERATED_BODY()

public:
    AEditorActorTagDisplayTestActor();

    virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

    UPROPERTY(EditAnywhere, Category = "Test")
    bool bTestBool = true;

    UPROPERTY(EditAnywhere, Category = "Test")
    int32 TestInteger = 42;

    UPROPERTY(EditAnywhere, Category = "Test")
    float TestFloat = 1.5f;

    UPROPERTY(EditAnywhere, Category = "Test")
    double TestDouble = 2.5;

    UPROPERTY(EditAnywhere, Category = "Test")
    uint32 TestUnsignedInteger = 84;

    UPROPERTY(EditAnywhere, Category = "Test")
    FName TestName = TEXT("TestName");

    UPROPERTY(EditAnywhere, Category = "Test")
    FString TestString = TEXT("Test String");

    UPROPERTY(EditAnywhere, Category = "Test")
    FText TestText = FText::FromString(TEXT("Test Text"));

    UPROPERTY(EditAnywhere, Category = "Test")
    EEditorActorTagDisplayTestEnum TestEnum = EEditorActorTagDisplayTestEnum::Second;

    UPROPERTY(EditAnywhere, Category = "Test")
    TObjectPtr<UObject> ObjectReference;

    UPROPERTY(EditAnywhere, Category = "Test")
    TSoftObjectPtr<UObject> SoftObjectReference;

    UPROPERTY(EditAnywhere, Category = "Test")
    FVector TestStruct = FVector(1.0f, 2.0f, 3.0f);

    UPROPERTY(EditAnywhere, Transient, Category = "Test")
    FString TransientString = TEXT("Do not display");

    UPROPERTY()
    int32 PrivateProperty = 7;

    UPROPERTY()
    FGameplayTagContainer GameplayTags;
};
