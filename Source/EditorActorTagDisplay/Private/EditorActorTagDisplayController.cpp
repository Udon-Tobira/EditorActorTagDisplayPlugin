// Copyright (c) 2026 metyatech. All rights reserved.

#include "EditorActorTagDisplayController.h"

#include "Debug/DebugDrawService.h"
#include "Editor.h"
#include "EditorActorTagDisplayLog.h"
#include "EditorActorTagDisplaySettings.h"
#include "EditorActorTagDisplayTemplateFormatter.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "ISettingsModule.h"
#include "Misc/TransactionObjectEvent.h"
#include "SceneView.h"
#include "ToolMenus.h"
#include "UObject/UObjectGlobals.h"
#include "CanvasItem.h"

#define LOCTEXT_NAMESPACE "EditorActorTagDisplay"

namespace
{
    constexpr float ScreenMargin = 16.0f;
}

void FEditorActorTagDisplayController::Initialize()
{
    if (bStarted)
    {
        return;
    }

    bStarted = true;
    RegisterDelegates();
    RegisterDebugDraw();
    RegisterMenusStartup();
    RebuildCache();
}

void FEditorActorTagDisplayController::Shutdown()
{
    if (!bStarted)
    {
        return;
    }

    UnregisterMenus();
    UnregisterDebugDraw();
    UnregisterDelegates();
    ActorCache.Empty();
    ResolvedRules.Empty();
    bStarted = false;
}

void FEditorActorTagDisplayController::RegisterDelegates()
{
    if (GEngine != nullptr)
    {
        LevelActorAddedHandle = GEngine->OnLevelActorAdded().AddRaw(this, &FEditorActorTagDisplayController::HandleLevelActorAdded);
        LevelActorDeletedHandle = GEngine->OnLevelActorDeleted().AddRaw(this, &FEditorActorTagDisplayController::HandleLevelActorDeleted);
        LevelActorListChangedHandle = GEngine->OnLevelActorListChanged().AddRaw(this, &FEditorActorTagDisplayController::HandleLevelActorListChanged);
        ActorMovedHandle = GEngine->OnActorMoved().AddRaw(this, &FEditorActorTagDisplayController::HandleActorMoved);
        LevelActorFolderChangedHandle = GEngine->OnLevelActorFolderChanged().AddRaw(this, &FEditorActorTagDisplayController::HandleLevelActorFolderChanged);
    }

    MapOpenedHandle = FEditorDelegates::OnMapOpened.AddRaw(this, &FEditorActorTagDisplayController::HandleMapOpened);
    ObjectPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FEditorActorTagDisplayController::HandleObjectPropertyChanged);
    ObjectTransactedHandle = FCoreUObjectDelegates::OnObjectTransacted.AddRaw(this, &FEditorActorTagDisplayController::HandleObjectTransacted);

    if (GEditor != nullptr)
    {
        BlueprintReinstancedHandle = GEditor->OnBlueprintReinstanced().AddRaw(this, &FEditorActorTagDisplayController::HandleBlueprintReinstanced);
    }
}

void FEditorActorTagDisplayController::UnregisterDelegates()
{
    if (GEngine != nullptr)
    {
        if (LevelActorAddedHandle.IsValid())
        {
            GEngine->OnLevelActorAdded().Remove(LevelActorAddedHandle);
        }
        if (LevelActorDeletedHandle.IsValid())
        {
            GEngine->OnLevelActorDeleted().Remove(LevelActorDeletedHandle);
        }
        if (LevelActorListChangedHandle.IsValid())
        {
            GEngine->OnLevelActorListChanged().Remove(LevelActorListChangedHandle);
        }
        if (ActorMovedHandle.IsValid())
        {
            GEngine->OnActorMoved().Remove(ActorMovedHandle);
        }
        if (LevelActorFolderChangedHandle.IsValid())
        {
            GEngine->OnLevelActorFolderChanged().Remove(LevelActorFolderChangedHandle);
        }
    }

    if (MapOpenedHandle.IsValid())
    {
        FEditorDelegates::OnMapOpened.Remove(MapOpenedHandle);
    }
    if (ObjectPropertyChangedHandle.IsValid())
    {
        FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(ObjectPropertyChangedHandle);
    }
    if (ObjectTransactedHandle.IsValid())
    {
        FCoreUObjectDelegates::OnObjectTransacted.Remove(ObjectTransactedHandle);
    }
    if (GEditor != nullptr && BlueprintReinstancedHandle.IsValid())
    {
        GEditor->OnBlueprintReinstanced().Remove(BlueprintReinstancedHandle);
    }

    LevelActorAddedHandle.Reset();
    LevelActorDeletedHandle.Reset();
    LevelActorListChangedHandle.Reset();
    ActorMovedHandle.Reset();
    LevelActorFolderChangedHandle.Reset();
    MapOpenedHandle.Reset();
    ObjectPropertyChangedHandle.Reset();
    ObjectTransactedHandle.Reset();
    BlueprintReinstancedHandle.Reset();
}

void FEditorActorTagDisplayController::RegisterDebugDraw()
{
    DebugDrawHandle = UDebugDrawService::Register(
        TEXT("Editor"),
        FDebugDrawDelegate::CreateRaw(this, &FEditorActorTagDisplayController::HandleDebugDraw));
}

void FEditorActorTagDisplayController::UnregisterDebugDraw()
{
    if (DebugDrawHandle.IsValid())
    {
        UDebugDrawService::Unregister(DebugDrawHandle);
        DebugDrawHandle.Reset();
    }
}

void FEditorActorTagDisplayController::RegisterMenusStartup()
{
    ToolMenusStartupCallbackHandle = UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FEditorActorTagDisplayController::RegisterMenus));
}

void FEditorActorTagDisplayController::UnregisterMenus()
{
    if (ToolMenusStartupCallbackHandle.IsValid())
    {
        UToolMenus::UnRegisterStartupCallback(ToolMenusStartupCallbackHandle);
        ToolMenusStartupCallbackHandle.Reset();
    }

    UToolMenus::UnregisterOwner(FToolMenuOwner(static_cast<void*>(this)));
}

void FEditorActorTagDisplayController::RebuildCache()
{
    ActorCache.Empty();
    ResolvedRules.Empty();
    WarnedRuleNames.Empty();
    WarnedPropertyKeys.Empty();

    const UEditorActorTagDisplayProjectSettings* ProjectSettings = GetDefault<UEditorActorTagDisplayProjectSettings>();
    if (ProjectSettings == nullptr)
    {
        RequestViewportRedraw();
        return;
    }

    EditorActorTagDisplayRuleMatcher::ResolveRules(ProjectSettings->Rules, ResolvedRules, WarnedRuleNames);

    UWorld* EditorWorld = GetEditorWorld();
    if (EditorWorld == nullptr || EditorWorld->WorldType != EWorldType::Editor)
    {
        RequestViewportRedraw();
        return;
    }

    for (TActorIterator<AActor> ActorIterator(EditorWorld); ActorIterator; ++ActorIterator)
    {
        RefreshActor(*ActorIterator);
    }

    RequestViewportRedraw();
}

void FEditorActorTagDisplayController::RefreshActor(AActor* Actor)
{
    if (Actor == nullptr)
    {
        return;
    }

    ActorCache.Remove(TWeakObjectPtr<AActor>(Actor));

    UWorld* EditorWorld = GetEditorWorld();
    if (!IsCacheableActor(Actor, EditorWorld))
    {
        return;
    }

    const int32 RuleIndex = EditorActorTagDisplayRuleMatcher::FindMatchingRule(*Actor, ResolvedRules);
    if (!ResolvedRules.IsValidIndex(RuleIndex))
    {
        return;
    }

    const UEditorActorTagDisplayProjectSettings* ProjectSettings = GetDefault<UEditorActorTagDisplayProjectSettings>();
    if (ProjectSettings == nullptr)
    {
        return;
    }

    const FActorMetadataOverlayRule& Rule = ResolvedRules[RuleIndex].Rule;
    const FString Template = Rule.DisplayTemplate.IsEmpty() ? ProjectSettings->DefaultDisplayTemplate : Rule.DisplayTemplate;
    FCachedActorMetadataOverlay Cached;
    Cached.Actor = Actor;
    Cached.ResolvedRuleIndex = RuleIndex;
    Cached.DisplayText = FEditorActorTagDisplayTemplateFormatter::Format(
        *Actor, Template, ProjectSettings->MaxPropertyValueLength, WarnedPropertyKeys);
    Cached.WorldBounds = Actor->GetComponentsBoundingBox(true);
    if (Cached.WorldBounds.IsValid)
    {
        Cached.WorldAnchor = Cached.WorldBounds.GetCenter() + FVector(0.0f, 0.0f, Cached.WorldBounds.GetExtent().Z);
    }
    else
    {
        Cached.WorldAnchor = Actor->GetActorLocation();
    }
    Cached.WorldAnchor += Rule.WorldOffset;
    ActorCache.Add(TWeakObjectPtr<AActor>(Actor), MoveTemp(Cached));
}

void FEditorActorTagDisplayController::RemoveActor(AActor* Actor)
{
    if (Actor != nullptr)
    {
        ActorCache.Remove(TWeakObjectPtr<AActor>(Actor));
        RequestViewportRedraw();
    }
}

bool FEditorActorTagDisplayController::IsCacheableActor(const AActor* Actor, const UWorld* EditorWorld) const
{
    if (Actor == nullptr || !IsValid(Actor) || EditorWorld == nullptr || Actor->GetWorld() != EditorWorld)
    {
        return false;
    }

    if (Actor->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject | RF_Transient) || Actor->IsActorBeingDestroyed())
    {
        return false;
    }

    return EditorWorld->WorldType == EWorldType::Editor;
}

void FEditorActorTagDisplayController::RequestViewportRedraw() const
{
    if (GEditor != nullptr)
    {
        GEditor->RedrawLevelEditingViewports();
    }
}

void FEditorActorTagDisplayController::HandleLevelActorAdded(AActor* Actor)
{
    RefreshActor(Actor);
    RequestViewportRedraw();
}

void FEditorActorTagDisplayController::HandleLevelActorDeleted(AActor* Actor)
{
    RemoveActor(Actor);
}

void FEditorActorTagDisplayController::HandleLevelActorListChanged()
{
    RebuildCache();
}

void FEditorActorTagDisplayController::HandleActorMoved(AActor* Actor)
{
    RefreshActor(Actor);
    RequestViewportRedraw();
}

void FEditorActorTagDisplayController::HandleLevelActorFolderChanged(const AActor* Actor, FName OldPath)
{
    RefreshActor(const_cast<AActor*>(Actor));
    RequestViewportRedraw();
}

void FEditorActorTagDisplayController::HandleMapOpened(const FString& Filename, bool bAsTemplate)
{
    RebuildCache();
}

void FEditorActorTagDisplayController::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
    if (bHandlingObjectEvent || Object == nullptr)
    {
        return;
    }

    TGuardValue<bool> EventGuard(bHandlingObjectEvent, true);
    if (Object->IsA<UEditorActorTagDisplayProjectSettings>())
    {
        RebuildCache();
        return;
    }
    if (Object->IsA<UEditorActorTagDisplayUserSettings>())
    {
        RequestViewportRedraw();
        return;
    }

    if (Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
    {
        return;
    }
    if (AActor* Actor = Cast<AActor>(Object))
    {
        RefreshActor(Actor);
        RequestViewportRedraw();
    }
    else if (UActorComponent* Component = Cast<UActorComponent>(Object))
    {
        RefreshActor(Component->GetOwner());
        RequestViewportRedraw();
    }
}

void FEditorActorTagDisplayController::HandleObjectTransacted(UObject* Object, const FTransactionObjectEvent& TransactionObjectEvent)
{
    if (bHandlingObjectEvent || Object == nullptr)
    {
        return;
    }

    TGuardValue<bool> EventGuard(bHandlingObjectEvent, true);
    if (Object->IsA<UEditorActorTagDisplayProjectSettings>())
    {
        RebuildCache();
        return;
    }
    if (Object->IsA<UEditorActorTagDisplayUserSettings>())
    {
        RequestViewportRedraw();
        return;
    }
    if (Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
    {
        return;
    }
    if (AActor* Actor = Cast<AActor>(Object))
    {
        RefreshActor(Actor);
        RequestViewportRedraw();
    }
    else if (UActorComponent* Component = Cast<UActorComponent>(Object))
    {
        RefreshActor(Component->GetOwner());
        RequestViewportRedraw();
    }
}

void FEditorActorTagDisplayController::HandleBlueprintReinstanced()
{
    RebuildCache();
}

void FEditorActorTagDisplayController::HandleDebugDraw(UCanvas* Canvas, APlayerController* PlayerController)
{
    if (Canvas == nullptr || Canvas->SceneView == nullptr || Canvas->SceneView->Family == nullptr || PlayerController != nullptr || GEditor == nullptr)
    {
        return;
    }

    UWorld* EditorWorld = GetEditorWorld();
    if (EditorWorld == nullptr || EditorWorld->WorldType != EWorldType::Editor || EditorWorld->Scene == nullptr ||
        Canvas->SceneView->Family->Scene != EditorWorld->Scene)
    {
        return;
    }

    const UEditorActorTagDisplayUserSettings* UserSettings = GetDefault<UEditorActorTagDisplayUserSettings>();
    const UEditorActorTagDisplayProjectSettings* ProjectSettings = GetDefault<UEditorActorTagDisplayProjectSettings>();
    if (EditorWorld == nullptr || UserSettings == nullptr || ProjectSettings == nullptr ||
        UserSettings->DisplayMode == EActorMetadataOverlayMode::Off)
    {
        return;
    }

    const UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    if (SmallFont == nullptr)
    {
        return;
    }

    for (const TPair<TWeakObjectPtr<AActor>, FCachedActorMetadataOverlay>& Pair : ActorCache)
    {
        const FCachedActorMetadataOverlay& Cached = Pair.Value;
        AActor* Actor = Cached.Actor.Get();
        if (Actor == nullptr || !IsValid(Actor) || Actor->GetWorld() != EditorWorld ||
            Actor->IsHiddenEd() || Actor->IsTemporarilyHiddenInEditor(true) ||
            !ResolvedRules.IsValidIndex(Cached.ResolvedRuleIndex))
        {
            continue;
        }

        const FActorMetadataOverlayRule& Rule = ResolvedRules[Cached.ResolvedRuleIndex].Rule;
        if (UserSettings->DisplayMode == EActorMetadataOverlayMode::Selected && !Actor->IsSelected())
        {
            continue;
        }
        if (Rule.bSelectedOnly && !Actor->IsSelected())
        {
            continue;
        }

        const float MaxDrawDistance = Rule.MaxDrawDistance > 0.0f ? Rule.MaxDrawDistance : UserSettings->GlobalMaxDrawDistance;
        if (MaxDrawDistance > 0.0f && FVector::DistSquared(Canvas->SceneView->ViewLocation, Cached.WorldAnchor) > FMath::Square(MaxDrawDistance))
        {
            continue;
        }

        const FVector ScreenPosition = Canvas->K2_Project(Cached.WorldAnchor);
        if (ScreenPosition.Z <= 0.0f || ScreenPosition.X < -ScreenMargin || ScreenPosition.X > Canvas->ClipX + ScreenMargin ||
            ScreenPosition.Y < -ScreenMargin || ScreenPosition.Y > Canvas->ClipY + ScreenMargin || Cached.DisplayText.IsEmpty())
        {
            continue;
        }

        Canvas->K2_DrawText(const_cast<UFont*>(SmallFont), Cached.DisplayText, FVector2D(ScreenPosition.X, ScreenPosition.Y),
                            FVector2D(UserSettings->TextScale, UserSettings->TextScale), Rule.DisplayColor, 0.0f,
                            FLinearColor::Transparent, FVector2D::ZeroVector, true, false,
                            UserSettings->bOutlined, ProjectSettings->OutlineColor);

        if (UserSettings->bDrawBoundingBoxes && Rule.bDrawBoundingBox && Cached.WorldBounds.IsValid)
        {
            DrawBoundingBox(Canvas, Cached.WorldBounds, Rule.DisplayColor);
        }
    }
}

void FEditorActorTagDisplayController::SetDisplayModeOff()
{
    UEditorActorTagDisplayUserSettings::Get()->DisplayMode = EActorMetadataOverlayMode::Off;
    UEditorActorTagDisplayUserSettings::Get()->SaveConfig();
    RequestViewportRedraw();
}

void FEditorActorTagDisplayController::SetDisplayModeSelected()
{
    UEditorActorTagDisplayUserSettings::Get()->DisplayMode = EActorMetadataOverlayMode::Selected;
    UEditorActorTagDisplayUserSettings::Get()->SaveConfig();
    RequestViewportRedraw();
}

void FEditorActorTagDisplayController::SetDisplayModeAll()
{
    UEditorActorTagDisplayUserSettings::Get()->DisplayMode = EActorMetadataOverlayMode::All;
    UEditorActorTagDisplayUserSettings::Get()->SaveConfig();
    RequestViewportRedraw();
}

bool FEditorActorTagDisplayController::CanExecuteMenuAction() const
{
    return true;
}

bool FEditorActorTagDisplayController::IsDisplayModeOff() const
{
    return UEditorActorTagDisplayUserSettings::Get()->DisplayMode == EActorMetadataOverlayMode::Off;
}

bool FEditorActorTagDisplayController::IsDisplayModeSelected() const
{
    return UEditorActorTagDisplayUserSettings::Get()->DisplayMode == EActorMetadataOverlayMode::Selected;
}

bool FEditorActorTagDisplayController::IsDisplayModeAll() const
{
    return UEditorActorTagDisplayUserSettings::Get()->DisplayMode == EActorMetadataOverlayMode::All;
}

void FEditorActorTagDisplayController::OpenProjectSettings()
{
    FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings")).ShowViewer(TEXT("Project"), TEXT("Plugins"), TEXT("ActorMetadataOverlay"));
}

void FEditorActorTagDisplayController::OpenEditorPreferences()
{
    FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings")).ShowViewer(TEXT("Editor"), TEXT("Plugins"), TEXT("ActorMetadataOverlay"));
}

void FEditorActorTagDisplayController::RegisterMenus()
{
    if (!bStarted || !UToolMenus::IsToolMenuUIEnabled())
    {
        return;
    }

    UToolMenus* ToolMenus = UToolMenus::Get();
    if (ToolMenus == nullptr)
    {
        return;
    }

    const FToolMenuOwner Owner(static_cast<void*>(this));
    FToolMenuOwnerScoped OwnerScoped(Owner);
    UToolMenu* Menu = ToolMenus->ExtendMenu(TEXT("LevelEditor.LevelViewportToolBar.Show"));
    if (Menu == nullptr)
    {
        return;
    }

    FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("ActorMetadataOverlay"), LOCTEXT("ActorMetadataOverlaySection", "Actor Metadata Overlay"));
    Section.AddMenuEntry(
        TEXT("ActorMetadataOverlay.Off"), LOCTEXT("ActorMetadataOverlayOff", "Off"), LOCTEXT("ActorMetadataOverlayOffTooltip", "Hide Actor Metadata Overlay text and bounds."),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FEditorActorTagDisplayController::SetDisplayModeOff),
                                FCanExecuteAction::CreateRaw(this, &FEditorActorTagDisplayController::CanExecuteMenuAction),
                                FIsActionChecked::CreateRaw(this, &FEditorActorTagDisplayController::IsDisplayModeOff)),
        EUserInterfaceActionType::RadioButton);
    Section.AddMenuEntry(
        TEXT("ActorMetadataOverlay.Selected"), LOCTEXT("ActorMetadataOverlaySelected", "Selected Actors"), LOCTEXT("ActorMetadataOverlaySelectedTooltip", "Show overlays for selected actors."),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FEditorActorTagDisplayController::SetDisplayModeSelected),
                                FCanExecuteAction::CreateRaw(this, &FEditorActorTagDisplayController::CanExecuteMenuAction),
                                FIsActionChecked::CreateRaw(this, &FEditorActorTagDisplayController::IsDisplayModeSelected)),
        EUserInterfaceActionType::RadioButton);
    Section.AddMenuEntry(
        TEXT("ActorMetadataOverlay.All"), LOCTEXT("ActorMetadataOverlayAll", "All Matching Actors"), LOCTEXT("ActorMetadataOverlayAllTooltip", "Show overlays for every actor matching a rule."),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FEditorActorTagDisplayController::SetDisplayModeAll),
                                FCanExecuteAction::CreateRaw(this, &FEditorActorTagDisplayController::CanExecuteMenuAction),
                                FIsActionChecked::CreateRaw(this, &FEditorActorTagDisplayController::IsDisplayModeAll)),
        EUserInterfaceActionType::RadioButton);
    Section.AddSeparator(TEXT("ActorMetadataOverlay.SettingsSeparator"));
    Section.AddMenuEntry(
        TEXT("ActorMetadataOverlay.ProjectSettings"), LOCTEXT("ActorMetadataOverlayProjectSettings", "Project Settings..."), LOCTEXT("ActorMetadataOverlayProjectSettingsTooltip", "Open Actor Metadata Overlay project settings."),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FEditorActorTagDisplayController::OpenProjectSettings),
                                FCanExecuteAction::CreateRaw(this, &FEditorActorTagDisplayController::CanExecuteMenuAction)));
    Section.AddMenuEntry(
        TEXT("ActorMetadataOverlay.EditorPreferences"), LOCTEXT("ActorMetadataOverlayEditorPreferences", "Editor Preferences..."), LOCTEXT("ActorMetadataOverlayEditorPreferencesTooltip", "Open Actor Metadata Overlay editor preferences."),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FEditorActorTagDisplayController::OpenEditorPreferences),
                                FCanExecuteAction::CreateRaw(this, &FEditorActorTagDisplayController::CanExecuteMenuAction)));
}

UWorld* FEditorActorTagDisplayController::GetEditorWorld()
{
    return GEditor == nullptr ? nullptr : GEditor->GetEditorWorldContext().World();
}

void FEditorActorTagDisplayController::DrawBoundingBox(UCanvas* Canvas, const FBox& Bounds, const FLinearColor& Color)
{
    if (Canvas == nullptr || Canvas->SceneView == nullptr || !Bounds.IsValid)
    {
        return;
    }

    const FVector Corners[8] = {
        FVector(Bounds.Min.X, Bounds.Min.Y, Bounds.Min.Z), FVector(Bounds.Max.X, Bounds.Min.Y, Bounds.Min.Z),
        FVector(Bounds.Max.X, Bounds.Max.Y, Bounds.Min.Z), FVector(Bounds.Min.X, Bounds.Max.Y, Bounds.Min.Z),
        FVector(Bounds.Min.X, Bounds.Min.Y, Bounds.Max.Z), FVector(Bounds.Max.X, Bounds.Min.Y, Bounds.Max.Z),
        FVector(Bounds.Max.X, Bounds.Max.Y, Bounds.Max.Z), FVector(Bounds.Min.X, Bounds.Max.Y, Bounds.Max.Z)};
    FVector ScreenCorners[8];
    for (int32 CornerIndex = 0; CornerIndex < UE_ARRAY_COUNT(Corners); ++CornerIndex)
    {
        ScreenCorners[CornerIndex] = Canvas->K2_Project(Corners[CornerIndex]);
    }

    const int32 Edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    for (int32 EdgeIndex = 0; EdgeIndex < UE_ARRAY_COUNT(Edges); ++EdgeIndex)
    {
        const FVector& Start = ScreenCorners[Edges[EdgeIndex][0]];
        const FVector& End = ScreenCorners[Edges[EdgeIndex][1]];
        if (Start.Z <= 0.0f || End.Z <= 0.0f)
        {
            continue;
        }

        FCanvasLineItem LineItem(FVector2D(Start.X, Start.Y), FVector2D(End.X, End.Y));
        LineItem.SetColor(Color);
        LineItem.LineThickness = 1.0f;
        Canvas->DrawItem(LineItem);
    }
}

#undef LOCTEXT_NAMESPACE
