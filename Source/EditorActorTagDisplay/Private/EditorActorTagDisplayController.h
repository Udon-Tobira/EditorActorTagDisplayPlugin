// Copyright (c) 2026 metyatech. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "EditorActorTagDisplayRuleMatcher.h"

class AActor;
class APlayerController;
class UActorComponent;
class UCanvas;
class UObject;
class UWorld;
struct FPropertyChangedEvent;
class FTransactionObjectEvent;

class FEditorActorTagDisplayController
{
public:
    void Initialize();
    void Shutdown();

private:
    struct FCachedActorMetadataOverlay
    {
        TWeakObjectPtr<AActor> Actor;
        int32 ResolvedRuleIndex = INDEX_NONE;
        FString DisplayText;
        FBox WorldBounds;
        FVector WorldAnchor = FVector::ZeroVector;
    };

    void RegisterDelegates();
    void UnregisterDelegates();
    void RegisterDebugDraw();
    void UnregisterDebugDraw();
    void RegisterMenusStartup();
    void RegisterMenus();
    void UnregisterMenus();

    void RebuildCache();
    void RefreshActor(AActor* Actor);
    void RemoveActor(AActor* Actor);
    bool IsCacheableActor(const AActor* Actor, const UWorld* EditorWorld) const;
    void RequestViewportRedraw() const;

    void HandleLevelActorAdded(AActor* Actor);
    void HandleLevelActorDeleted(AActor* Actor);
    void HandleLevelActorListChanged();
    void HandleActorMoved(AActor* Actor);
    void HandleLevelActorFolderChanged(const AActor* Actor, FName OldPath);
    void HandleMapOpened(const FString& Filename, bool bAsTemplate);
    void HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
    void HandleObjectTransacted(UObject* Object, const FTransactionObjectEvent& TransactionObjectEvent);
    void HandleBlueprintReinstanced();
    void HandleDebugDraw(UCanvas* Canvas, APlayerController* PlayerController);
    static void DrawBoundingBox(UCanvas* Canvas, const FBox& Bounds, const FLinearColor& Color);

    void SetDisplayModeOff();
    void SetDisplayModeSelected();
    void SetDisplayModeAll();
    bool CanExecuteMenuAction() const;
    bool IsDisplayModeOff() const;
    bool IsDisplayModeSelected() const;
    bool IsDisplayModeAll() const;
    void OpenProjectSettings();
    void OpenEditorPreferences();

    static UWorld* GetEditorWorld();

    bool bStarted = false;
    bool bHandlingObjectEvent = false;
    FDelegateHandle DebugDrawHandle;
    FDelegateHandle ToolMenusStartupCallbackHandle;
    FDelegateHandle LevelActorAddedHandle;
    FDelegateHandle LevelActorDeletedHandle;
    FDelegateHandle LevelActorListChangedHandle;
    FDelegateHandle ActorMovedHandle;
    FDelegateHandle LevelActorFolderChangedHandle;
    FDelegateHandle MapOpenedHandle;
    FDelegateHandle ObjectPropertyChangedHandle;
    FDelegateHandle ObjectTransactedHandle;
    FDelegateHandle BlueprintReinstancedHandle;

    TArray<FResolvedActorMetadataOverlayRule> ResolvedRules;
    TMap<TWeakObjectPtr<AActor>, FCachedActorMetadataOverlay> ActorCache;
    TSet<FName> WarnedRuleNames;
    TSet<FString> WarnedPropertyKeys;
};
