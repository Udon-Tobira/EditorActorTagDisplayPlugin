// Copyright (c) 2026 metyatech. All rights reserved.

#include "ActorMetadataOverlayController.h"

#include "Debug/DebugDrawService.h"
#include "Editor.h"
#include "ActorMetadataOverlayLog.h"
#include "ActorMetadataOverlaySettings.h"
#include "ActorMetadataOverlayTemplateFormatter.h"
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

#define LOCTEXT_NAMESPACE "ActorMetadataOverlay"

namespace
{
    constexpr float ScreenMargin = 16.0f;
}

void FActorMetadataOverlayController::Initialize()
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

void FActorMetadataOverlayController::Shutdown()
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

void FActorMetadataOverlayController::RegisterDelegates()
{
    if (GEngine != nullptr)
    {
        LevelActorAddedHandle = GEngine->OnLevelActorAdded().AddRaw(this, &FActorMetadataOverlayController::HandleLevelActorAdded);
        LevelActorDeletedHandle = GEngine->OnLevelActorDeleted().AddRaw(this, &FActorMetadataOverlayController::HandleLevelActorDeleted);
        LevelActorListChangedHandle = GEngine->OnLevelActorListChanged().AddRaw(this, &FActorMetadataOverlayController::HandleLevelActorListChanged);
        ActorMovedHandle = GEngine->OnActorMoved().AddRaw(this, &FActorMetadataOverlayController::HandleActorMoved);
        LevelActorFolderChangedHandle = GEngine->OnLevelActorFolderChanged().AddRaw(this, &FActorMetadataOverlayController::HandleLevelActorFolderChanged);
    }

    MapOpenedHandle = FEditorDelegates::OnMapOpened.AddRaw(this, &FActorMetadataOverlayController::HandleMapOpened);
    ObjectPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FActorMetadataOverlayController::HandleObjectPropertyChanged);
    ObjectTransactedHandle = FCoreUObjectDelegates::OnObjectTransacted.AddRaw(this, &FActorMetadataOverlayController::HandleObjectTransacted);

    if (GEditor != nullptr)
    {
        BlueprintReinstancedHandle = GEditor->OnBlueprintReinstanced().AddRaw(this, &FActorMetadataOverlayController::HandleBlueprintReinstanced);
    }
}

void FActorMetadataOverlayController::UnregisterDelegates()
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

void FActorMetadataOverlayController::RegisterDebugDraw()
{
    DebugDrawHandle = UDebugDrawService::Register(
        TEXT("Editor"),
        FDebugDrawDelegate::CreateRaw(this, &FActorMetadataOverlayController::HandleDebugDraw));
}

void FActorMetadataOverlayController::UnregisterDebugDraw()
{
    if (DebugDrawHandle.IsValid())
    {
        UDebugDrawService::Unregister(DebugDrawHandle);
        DebugDrawHandle.Reset();
    }
}

void FActorMetadataOverlayController::RegisterMenusStartup()
{
    ToolMenusStartupCallbackHandle = UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FActorMetadataOverlayController::RegisterMenus));
}

void FActorMetadataOverlayController::UnregisterMenus()
{
    if (ToolMenusStartupCallbackHandle.IsValid())
    {
        UToolMenus::UnRegisterStartupCallback(ToolMenusStartupCallbackHandle);
        ToolMenusStartupCallbackHandle.Reset();
    }

    UToolMenus::UnregisterOwner(FToolMenuOwner(static_cast<void*>(this)));
}

void FActorMetadataOverlayController::RebuildCache()
{
    ActorCache.Empty();
    ResolvedRules.Empty();
    WarnedRuleNames.Empty();
    WarnedPropertyKeys.Empty();

    const UActorMetadataOverlayProjectSettings* ProjectSettings = GetDefault<UActorMetadataOverlayProjectSettings>();
    if (ProjectSettings == nullptr)
    {
        RequestViewportRedraw();
        return;
    }

    ActorMetadataOverlayRuleMatcher::ResolveRules(ProjectSettings->Rules, ResolvedRules, WarnedRuleNames);

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

void FActorMetadataOverlayController::RefreshActor(AActor* Actor)
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

    const int32 RuleIndex = ActorMetadataOverlayRuleMatcher::FindMatchingRule(*Actor, ResolvedRules);
    if (!ResolvedRules.IsValidIndex(RuleIndex))
    {
        return;
    }

    const UActorMetadataOverlayProjectSettings* ProjectSettings = GetDefault<UActorMetadataOverlayProjectSettings>();
    if (ProjectSettings == nullptr)
    {
        return;
    }

    const FActorMetadataOverlayRule& Rule = ResolvedRules[RuleIndex].Rule;
    const FString Template = Rule.DisplayTemplate.IsEmpty() ? ProjectSettings->DefaultDisplayTemplate : Rule.DisplayTemplate;
    FCachedActorMetadataOverlay Cached;
    Cached.Actor = Actor;
    Cached.ResolvedRuleIndex = RuleIndex;
    Cached.DisplayText = FActorMetadataOverlayTemplateFormatter::Format(
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

void FActorMetadataOverlayController::RemoveActor(AActor* Actor)
{
    if (Actor != nullptr)
    {
        ActorCache.Remove(TWeakObjectPtr<AActor>(Actor));
        RequestViewportRedraw();
    }
}

bool FActorMetadataOverlayController::IsCacheableActor(const AActor* Actor, const UWorld* EditorWorld) const
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

void FActorMetadataOverlayController::RequestViewportRedraw() const
{
    if (GEditor != nullptr)
    {
        GEditor->RedrawLevelEditingViewports();
    }
}

void FActorMetadataOverlayController::HandleLevelActorAdded(AActor* Actor)
{
    RefreshActor(Actor);
    RequestViewportRedraw();
}

void FActorMetadataOverlayController::HandleLevelActorDeleted(AActor* Actor)
{
    RemoveActor(Actor);
}

void FActorMetadataOverlayController::HandleLevelActorListChanged()
{
    RebuildCache();
}

void FActorMetadataOverlayController::HandleActorMoved(AActor* Actor)
{
    RefreshActor(Actor);
    RequestViewportRedraw();
}

void FActorMetadataOverlayController::HandleLevelActorFolderChanged(const AActor* Actor, FName OldPath)
{
    RefreshActor(const_cast<AActor*>(Actor));
    RequestViewportRedraw();
}

void FActorMetadataOverlayController::HandleMapOpened(const FString& Filename, bool bAsTemplate)
{
    RebuildCache();
}

void FActorMetadataOverlayController::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
    if (bHandlingObjectEvent || Object == nullptr)
    {
        return;
    }

    TGuardValue<bool> EventGuard(bHandlingObjectEvent, true);
    if (Object->IsA<UActorMetadataOverlayProjectSettings>())
    {
        RebuildCache();
        return;
    }
    if (Object->IsA<UActorMetadataOverlayUserSettings>())
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

void FActorMetadataOverlayController::HandleObjectTransacted(UObject* Object, const FTransactionObjectEvent& TransactionObjectEvent)
{
    if (bHandlingObjectEvent || Object == nullptr)
    {
        return;
    }

    TGuardValue<bool> EventGuard(bHandlingObjectEvent, true);
    if (Object->IsA<UActorMetadataOverlayProjectSettings>())
    {
        RebuildCache();
        return;
    }
    if (Object->IsA<UActorMetadataOverlayUserSettings>())
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

void FActorMetadataOverlayController::HandleBlueprintReinstanced()
{
    RebuildCache();
}

void FActorMetadataOverlayController::HandleDebugDraw(UCanvas* Canvas, APlayerController* PlayerController)
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

    const UActorMetadataOverlayUserSettings* UserSettings = GetDefault<UActorMetadataOverlayUserSettings>();
    const UActorMetadataOverlayProjectSettings* ProjectSettings = GetDefault<UActorMetadataOverlayProjectSettings>();
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

void FActorMetadataOverlayController::SetDisplayModeOff()
{
    UActorMetadataOverlayUserSettings::Get()->DisplayMode = EActorMetadataOverlayMode::Off;
    UActorMetadataOverlayUserSettings::Get()->SaveConfig();
    RequestViewportRedraw();
}

void FActorMetadataOverlayController::SetDisplayModeSelected()
{
    UActorMetadataOverlayUserSettings::Get()->DisplayMode = EActorMetadataOverlayMode::Selected;
    UActorMetadataOverlayUserSettings::Get()->SaveConfig();
    RequestViewportRedraw();
}

void FActorMetadataOverlayController::SetDisplayModeAll()
{
    UActorMetadataOverlayUserSettings::Get()->DisplayMode = EActorMetadataOverlayMode::All;
    UActorMetadataOverlayUserSettings::Get()->SaveConfig();
    RequestViewportRedraw();
}

bool FActorMetadataOverlayController::CanExecuteMenuAction() const
{
    return true;
}

bool FActorMetadataOverlayController::IsDisplayModeOff() const
{
    return UActorMetadataOverlayUserSettings::Get()->DisplayMode == EActorMetadataOverlayMode::Off;
}

bool FActorMetadataOverlayController::IsDisplayModeSelected() const
{
    return UActorMetadataOverlayUserSettings::Get()->DisplayMode == EActorMetadataOverlayMode::Selected;
}

bool FActorMetadataOverlayController::IsDisplayModeAll() const
{
    return UActorMetadataOverlayUserSettings::Get()->DisplayMode == EActorMetadataOverlayMode::All;
}

void FActorMetadataOverlayController::OpenProjectSettings()
{
    FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings")).ShowViewer(TEXT("Project"), TEXT("Plugins"), TEXT("ActorMetadataOverlay"));
}

void FActorMetadataOverlayController::OpenEditorPreferences()
{
    FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings")).ShowViewer(TEXT("Editor"), TEXT("Plugins"), TEXT("ActorMetadataOverlay"));
}

void FActorMetadataOverlayController::RegisterMenus()
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
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FActorMetadataOverlayController::SetDisplayModeOff),
                                FCanExecuteAction::CreateRaw(this, &FActorMetadataOverlayController::CanExecuteMenuAction),
                                FIsActionChecked::CreateRaw(this, &FActorMetadataOverlayController::IsDisplayModeOff)),
        EUserInterfaceActionType::RadioButton);
    Section.AddMenuEntry(
        TEXT("ActorMetadataOverlay.Selected"), LOCTEXT("ActorMetadataOverlaySelected", "Selected Actors"), LOCTEXT("ActorMetadataOverlaySelectedTooltip", "Show overlays for selected actors."),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FActorMetadataOverlayController::SetDisplayModeSelected),
                                FCanExecuteAction::CreateRaw(this, &FActorMetadataOverlayController::CanExecuteMenuAction),
                                FIsActionChecked::CreateRaw(this, &FActorMetadataOverlayController::IsDisplayModeSelected)),
        EUserInterfaceActionType::RadioButton);
    Section.AddMenuEntry(
        TEXT("ActorMetadataOverlay.All"), LOCTEXT("ActorMetadataOverlayAll", "All Matching Actors"), LOCTEXT("ActorMetadataOverlayAllTooltip", "Show overlays for every actor matching a rule."),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FActorMetadataOverlayController::SetDisplayModeAll),
                                FCanExecuteAction::CreateRaw(this, &FActorMetadataOverlayController::CanExecuteMenuAction),
                                FIsActionChecked::CreateRaw(this, &FActorMetadataOverlayController::IsDisplayModeAll)),
        EUserInterfaceActionType::RadioButton);
    Section.AddSeparator(TEXT("ActorMetadataOverlay.SettingsSeparator"));
    Section.AddMenuEntry(
        TEXT("ActorMetadataOverlay.ProjectSettings"), LOCTEXT("ActorMetadataOverlayProjectSettings", "Project Settings..."), LOCTEXT("ActorMetadataOverlayProjectSettingsTooltip", "Open Actor Metadata Overlay project settings."),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FActorMetadataOverlayController::OpenProjectSettings),
                                FCanExecuteAction::CreateRaw(this, &FActorMetadataOverlayController::CanExecuteMenuAction)));
    Section.AddMenuEntry(
        TEXT("ActorMetadataOverlay.EditorPreferences"), LOCTEXT("ActorMetadataOverlayEditorPreferences", "Editor Preferences..."), LOCTEXT("ActorMetadataOverlayEditorPreferencesTooltip", "Open Actor Metadata Overlay editor preferences."),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FActorMetadataOverlayController::OpenEditorPreferences),
                                FCanExecuteAction::CreateRaw(this, &FActorMetadataOverlayController::CanExecuteMenuAction)));
}

UWorld* FActorMetadataOverlayController::GetEditorWorld()
{
    return GEditor == nullptr ? nullptr : GEditor->GetEditorWorldContext().World();
}

void FActorMetadataOverlayController::DrawBoundingBox(UCanvas* Canvas, const FBox& Bounds, const FLinearColor& Color)
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
