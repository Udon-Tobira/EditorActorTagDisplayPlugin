#include "EditorActorTagDisplayModule.h"
#include "EditorActorTagDisplaySettings.h"
#include "EditorActorTagDisplayActor.h"
#include "EditorActorTagDisplayLog.h"
#include "Engine/World.h"
#include "Serialization/MemoryLayout.h"
#include "ToolMenus.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "EditorViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/AssetManager.h"

#define LOCTEXT_NAMESPACE "EditorActorTagDisplay"

void FEditorActorTagDisplayModule::StartupModule()
{
    RegisterDebugDrawDelegate();
    FEditorActorTagDisplayModule::AddViewportShowFlagExtension();
    
    // フォントサイズ変更デリゲートを購読
    if (UEditorActorTagDisplaySettings* Settings = UEditorActorTagDisplaySettings::Get())
    {
        TextSizeChangedDelegateHandle = Settings->GetOnTextSizeChangedDelegate().AddLambda([this](float /*NewTextSize*/)
        -> void {
            UpdateAllTextActorSizes();
        });
        
        OutlineWidthChangedDelegateHandle = Settings->GetOnOutlineWidthChangedDelegate().AddLambda([this](float /*NewOutlineWidth*/)
        -> void {
            UpdateAllTextActorOutlineWidth();
        });
    }
}

void FEditorActorTagDisplayModule::ShutdownModule()
{
    UnregisterDebugDrawDelegate();
    RemoveViewportShowFlagExtension();
    
    // デリゲートハンドルをリセット
    // デリゲートは自動的に破棄されるため、明示的な削除は不要。
    TextSizeChangedDelegateHandle.Reset();
    OutlineWidthChangedDelegateHandle.Reset();
}

void FEditorActorTagDisplayModule::RegisterDebugDrawDelegate()
{
    // FTickerを使用してTextRenderComponentを更新
    TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float  /*DeltaTime*/)
    -> bool {
        UpdateTextActors();
        return true; // 継続実行
    }));
}

void FEditorActorTagDisplayModule::UnregisterDebugDrawDelegate()
{
    if (TickDelegateHandle.IsValid())
    {
        FTSTicker::RemoveTicker(TickDelegateHandle);
    }
    
    CleanupTextActors();
}

void FEditorActorTagDisplayModule::UpdateTextActors()
{
    const UEditorActorTagDisplaySettings* Settings = UEditorActorTagDisplaySettings::Get();
    if (Settings == nullptr || !Settings->IsTagDisplayEnabled())
    {
        CleanupTextActors();
        return;
    }

    UWorld* World = FEditorActorTagDisplayModule::GetEditorWorld();
    if (World == nullptr)
    {
        CleanupTextActors();
        return;
    }

    TSet<TWeakObjectPtr<AActor>> ProcessedActors;
    ProcessActorsInWorld(World, Settings, ProcessedActors);
    RemoveUnusedTextActors(ProcessedActors);
}

auto FEditorActorTagDisplayModule::GetEditorWorld() -> UWorld*
{
    if (GEditor == nullptr)
    {
        return nullptr;
    }
    return GEditor->GetEditorWorldContext().World();
}

void FEditorActorTagDisplayModule::ProcessActorsInWorld(UWorld* World, const UEditorActorTagDisplaySettings* Settings, TSet<TWeakObjectPtr<AActor>>& ProcessedActors)
{
    check(World != nullptr); // NOLINT
    check(Settings != nullptr); // NOLINT

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor == nullptr || !IsValid(Actor) || Actor->Tags.IsEmpty())
        {
            continue;
        }

        ProcessActorIfMatched(Actor, Settings, ProcessedActors);
    }
}

void FEditorActorTagDisplayModule::ProcessActorIfMatched(AActor* Actor, const UEditorActorTagDisplaySettings* Settings, TSet<TWeakObjectPtr<AActor>>& ProcessedActors)
{
    check(Actor != nullptr); // NOLINT
    check(Settings != nullptr); // NOLINT

    for (const FActorClassTagDisplayConfig& Config : Settings->GetClassConfigs())
    {
        if (Config.ActorClass.IsValid() && Actor->IsA(Config.ActorClass.Get()))
        {
            ProcessedActors.Add(Actor);
            CreateOrUpdateTextActor(Actor, Config);
            break;
        }
    }
}

void FEditorActorTagDisplayModule::CreateOrUpdateTextActor(AActor* Actor, const FActorClassTagDisplayConfig& Config)
{
    check(Actor != nullptr); // NOLINT

    FString CombinedTags = FEditorActorTagDisplayModule::CombineActorTags(Actor);
    if (CombinedTags.IsEmpty())
    {
        return;
    }

    AEditorActorTagDisplayActor* TextActor = GetOrCreateTextActor(Actor);
    if (TextActor == nullptr)
    {
        return;
    }

    FEditorActorTagDisplayModule::UpdateTextActorProperties(TextActor, CombinedTags, Config, Actor);
}

auto FEditorActorTagDisplayModule::CombineActorTags(AActor* Actor) -> FString
{
    check(Actor != nullptr); // NOLINT

    TArray<FString> TagStrings;
    for (const FName& Tag : Actor->Tags)
    {
        TagStrings.Add(Tag.ToString());
    }
    return FString::Join(TagStrings, TEXT("\n"));
}

auto FEditorActorTagDisplayModule::GetOrCreateTextActor(AActor* Actor) -> AEditorActorTagDisplayActor*
{
    check(Actor != nullptr); // NOLINT

    TWeakObjectPtr<AEditorActorTagDisplayActor>* ExistingActorPtr = TextActorMap.Find(Actor);
    if (ExistingActorPtr != nullptr && ExistingActorPtr->IsValid())
    {
        return ExistingActorPtr->Get();
    }

    UWorld* World = Actor->GetWorld();
    if (World == nullptr)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName(*FString::Printf(TEXT("TagDisplayActor_%s"), *Actor->GetName()));
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.bHideFromSceneOutliner = true;
    SpawnParams.ObjectFlags = RF_Transient;

    auto* const TextActor = World->SpawnActor<AEditorActorTagDisplayActor>(SpawnParams);
    if (TextActor == nullptr)
    {
        return nullptr;
    }

    FEditorActorTagDisplayModule::SetupTextActor(TextActor);
    TextActorMap.Add(Actor, TextActor);
    return TextActor;
}

void FEditorActorTagDisplayModule::SetupTextActor(AEditorActorTagDisplayActor* TextActor)
{
    check(TextActor != nullptr); // NOLINT

    UTextRenderComponent* TextComponent = TextActor->GetTextRenderComponent();
    if (TextComponent == nullptr)
    {
        return;
    }

    const UEditorActorTagDisplaySettings* Settings = UEditorActorTagDisplaySettings::Get();
    check(Settings != nullptr); // NOLINT
    const float TextSize = Settings->GetTextSize();

    TextComponent->SetMobility(EComponentMobility::Movable);
    TextComponent->SetHorizontalAlignment(EHTA_Center);
    TextComponent->SetWorldSize(TextSize);
    TextComponent->SetVisibility(true);
    
    // プラグイン内のマテリアルを設定
    FEditorActorTagDisplayModule::SetTextMaterial(TextComponent);
}

void FEditorActorTagDisplayModule::UpdateTextActorProperties(AEditorActorTagDisplayActor* TextActor, const FString& CombinedTags, const FActorClassTagDisplayConfig& Config, AActor* Actor)
{
    check(TextActor != nullptr); // NOLINT
    check(Actor != nullptr); // NOLINT

    UTextRenderComponent* TextComponent = TextActor->GetTextRenderComponent();
    if (TextComponent == nullptr)
    {
        return;
    }

    TextComponent->SetText(FText::FromString(CombinedTags));
    TextComponent->SetTextRenderColor(Config.DisplayColor.ToFColor(true));
    
    FVector TextPosition = Actor->GetActorLocation();

    const FBox ComponentsBounds = Actor->GetComponentsBoundingBox(false); // コリジョンコンポーネントのみ
    if (ComponentsBounds.IsValid != 0U)
    {
        FVector BoundingCenter;
        FVector BoundingExtents;
        ComponentsBounds.GetCenterAndExtents(BoundingCenter, BoundingExtents);

        const auto HalfHeight = static_cast<float>(FVector::DotProduct(BoundingExtents, FVector::UpVector));
        TextPosition = BoundingCenter + FVector::UpVector * HalfHeight;
    }
    else
    {
        // コリジョンコンポーネントがない場合は、アクターの原点位置を使用
        TextPosition = Actor->GetActorLocation();
    }

    TextPosition += Config.PositionOffset; // クラスごとの位置オフセットを適用
    TextActor->SetActorLocation(TextPosition);

    FEditorActorTagDisplayModule::UpdateTextActorRotation(TextActor, TextPosition);
}

auto FEditorActorTagDisplayModule::GetCameraLocation() -> FVector
{
    // PIE実行中かどうかを確認
    if (GEditor != nullptr && GEditor->IsPlaySessionInProgress())
    {
        // PIE実行中はゲームのカメラ位置を取得
        UWorld* PIEWorld = (GEditor->GetPIEWorldContext() != nullptr) ? GEditor->GetPIEWorldContext()->World() : nullptr;
        if (PIEWorld != nullptr)
        {
            // プレイヤーコントローラーからカメラ位置を取得
            for (FConstPlayerControllerIterator Iterator = PIEWorld->GetPlayerControllerIterator(); Iterator; ++Iterator)
            {
                APlayerController* PlayerController = Iterator->Get();
                if (PlayerController != nullptr && PlayerController->PlayerCameraManager != nullptr)
                {
                    return PlayerController->PlayerCameraManager->GetCameraLocation();
                }
            }
        }
    }
    
    // エディター実行中はエディタービューポートのカメラ位置を取得
    if (GEditor != nullptr && GEditor->GetActiveViewport() != nullptr)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        const auto* const ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
        if (ViewportClient != nullptr)
        {
            return ViewportClient->GetViewLocation();
        }
    }
    
    return FVector::ZeroVector;
}

void FEditorActorTagDisplayModule::UpdateTextActorRotation(AEditorActorTagDisplayActor* TextActor, const FVector& TextPosition)
{
    check(TextActor != nullptr); // NOLINT

    FVector CameraLocation = FEditorActorTagDisplayModule::GetCameraLocation();
    if (CameraLocation.IsZero())
    {
        return;
    }

    FVector DirectionToCamera = (CameraLocation - TextPosition).GetSafeNormal();
    FRotator LookAtRotation = DirectionToCamera.Rotation();
    TextActor->SetActorRotation(LookAtRotation);
}

void FEditorActorTagDisplayModule::RemoveUnusedTextActors(const TSet<TWeakObjectPtr<AActor>>& ProcessedActors)
{
    TArray<TWeakObjectPtr<AActor>> ToRemove;
    
    for (auto& Pair : TextActorMap)
    {
        if (!Pair.Key.IsValid() || !ProcessedActors.Contains(Pair.Key))
        {
            if (Pair.Value.IsValid())
            {
                Pair.Value->Destroy();
            }
            ToRemove.Add(Pair.Key);
        }
    }
    
    for (const auto& Key : ToRemove)
    {
        TextActorMap.Remove(Key);
    }
}

void FEditorActorTagDisplayModule::CleanupTextActors()
{
    for (auto& Pair : TextActorMap)
    {
        if (Pair.Value.IsValid())
        {
            Pair.Value->Destroy();
        }
    }
    TextActorMap.Empty();
}

void FEditorActorTagDisplayModule::AddViewportShowFlagExtension()
{
    // メニュー登録を遅延実行
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
    -> void {
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelViewportToolBar.Show");
        if (Menu)
        {
            FToolMenuSection& Section = Menu->FindOrAddSection("LevelViewportShowFlags");
            
            Section.AddMenuEntry(
                "ActorTags",
                LOCTEXT("ActorTags", "Actor Tags"),
                LOCTEXT("ActorTagsTooltip", "Toggle display of actor tags in viewport"),
                FSlateIcon(),
                FUIAction(
                    FExecuteAction::CreateLambda([]()
                    -> void {
                        if (UEditorActorTagDisplaySettings* Settings = UEditorActorTagDisplaySettings::Get())
                        {
                            Settings->SetTagDisplayEnabled(!Settings->IsTagDisplayEnabled());
                        }
                    }),
                    FCanExecuteAction::CreateLambda([]() -> bool { return true; }),
                    FIsActionChecked::CreateLambda([]()
                    -> bool {
                        if (const UEditorActorTagDisplaySettings* Settings = UEditorActorTagDisplaySettings::Get())
                        {
                            return Settings->IsTagDisplayEnabled();
                        }
                        return false;
                    })
                ),
                EUserInterfaceActionType::ToggleButton
            );
        }
    }));
}

void FEditorActorTagDisplayModule::RemoveViewportShowFlagExtension()
{
    // メニュー拡張の登録解除は自動的に行われる
}

void FEditorActorTagDisplayModule::SetTextMaterial(UTextRenderComponent* TextComponent)
{
    check(TextComponent != nullptr); // NOLINT

    const FSoftObjectPath MaterialPath(TEXT_MATERIAL_PATH);
    UMaterialInterface* TextMaterial = Cast<UMaterialInterface>(MaterialPath.TryLoad());
    
    if (TextMaterial != nullptr)
    {
        ApplyMaterial(TextComponent, TextMaterial);
        return;
    }
    
    UE_LOG(LogEditorActorTagDisplay, Error, TEXT("Failed to load text material from %s"), TEXT_MATERIAL_PATH); // NOLINT
}

void FEditorActorTagDisplayModule::ApplyMaterial(UTextRenderComponent* TextComponent, UMaterialInterface* TextMaterial)
{
    check(TextComponent != nullptr); // NOLINT
    check(TextMaterial != nullptr); // NOLINT

    UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(TextMaterial, TextComponent->GetOwner());
    if (DynamicMaterial == nullptr)
    {
        TextComponent->SetTextMaterial(TextMaterial);
        return;
    }

    const UEditorActorTagDisplaySettings* Settings = UEditorActorTagDisplaySettings::Get();
    if (Settings != nullptr)
    {
        DynamicMaterial->SetScalarParameterValue(TEXT("OutlineWidth"), Settings->GetOutlineWidth());
    }
    TextComponent->SetTextMaterial(DynamicMaterial);
}

void FEditorActorTagDisplayModule::UpdateAllTextActorSizes()
{
    const UEditorActorTagDisplaySettings* Settings = UEditorActorTagDisplaySettings::Get();
    if (Settings == nullptr)
    {
        return;
    }
    
    const float NewTextSize = Settings->GetTextSize();
    
    // 既存のすべてのTextActorのサイズを更新
    for (auto& Pair : TextActorMap)
    {
        if (Pair.Value.IsValid())
        {
            AEditorActorTagDisplayActor* TextActor = Pair.Value.Get();
            UTextRenderComponent* TextComponent = TextActor->GetTextRenderComponent();
            if (TextComponent != nullptr)
            {
                TextComponent->SetWorldSize(NewTextSize);
            }
        }
    }
}

void FEditorActorTagDisplayModule::UpdateAllTextActorOutlineWidth()
{
    const UEditorActorTagDisplaySettings* Settings = UEditorActorTagDisplaySettings::Get();
    if (Settings == nullptr)
    {
        return;
    }
    
    const float NewOutlineWidth = Settings->GetOutlineWidth();
    
    // 既存のすべてのTextActorのOutlineWidthを更新
    for (auto& Pair : TextActorMap)
    {
        if (Pair.Value.IsValid())
        {
            AEditorActorTagDisplayActor* TextActor = Pair.Value.Get();
            UTextRenderComponent* TextComponent = TextActor->GetTextRenderComponent();
            if (TextComponent != nullptr)
            {
                // 既存のマテリアルインスタンスのパラメータを更新
                UMaterialInterface* Material = TextComponent->GetMaterial(0);
                if (UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Material))
                {
                    DynamicMaterial->SetScalarParameterValue(TEXT("OutlineWidth"), NewOutlineWidth);
                }
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEditorActorTagDisplayModule, EditorActorTagDisplay) // NOLINT
