#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// 前方宣言
class AActor;
class UTextRenderComponent;
class UWorld;
class UEditorActorTagDisplaySettings;
class AEditorActorTagDisplayActor;
class UMaterialInterface;
struct FActorClassTagDisplayConfig;

class FEditorActorTagDisplayModule : public IModuleInterface
{
public:
    // マテリアルパス定数
    static constexpr const TCHAR *TEXT_MATERIAL_PATH =
        TEXT("/EditorActorTagDisplay/Materials/M_TextMaterial.M_TextMaterial");

    // IModuleInterface implementation
    auto StartupModule() -> void override;
    auto ShutdownModule() -> void override;

private:
    // モジュール初期化・終了関連
    auto RegisterDebugDrawDelegate() -> void;
    auto UnregisterDebugDrawDelegate() -> void;
    static auto AddViewportShowFlagExtension() -> void;
    auto RemoveViewportShowFlagExtension() -> void;

    // テキストアクター管理
    auto UpdateTextActors() -> void;
    auto CleanupTextActors() -> void;
    auto RemoveUnusedTextActors(const TSet<TWeakObjectPtr<AActor>> &ProcessedActors) -> void;

    // ワールド・アクター処理
    [[nodiscard]] static auto GetEditorWorld() -> UWorld *;
    auto ProcessActorsInWorld(UWorld *World, const UEditorActorTagDisplaySettings *Settings,
                              TSet<TWeakObjectPtr<AActor>> &ProcessedActors) -> void;
    auto ProcessActorIfMatched(AActor *Actor, const UEditorActorTagDisplaySettings *Settings,
                               TSet<TWeakObjectPtr<AActor>> &ProcessedActors) -> void;

    // テキストアクター作成・更新
    auto CreateOrUpdateTextActor(AActor *Actor, const FActorClassTagDisplayConfig &Config) -> void;
    auto GetOrCreateTextActor(AActor *Actor) -> AEditorActorTagDisplayActor *;
    static auto SetupTextActor(AEditorActorTagDisplayActor *TextActor) -> void;
    static auto UpdateTextActorProperties(AEditorActorTagDisplayActor *TextActor, const FString &CombinedTags,
                                          const FActorClassTagDisplayConfig &Config, AActor *Actor) -> void;
    static auto UpdateTextActorRotation(AEditorActorTagDisplayActor *TextActor, const FVector &TextPosition) -> void;

    // マテリアル設定
    static auto SetTextMaterial(UTextRenderComponent *TextComponent) -> void;
    static auto ApplyMaterial(UTextRenderComponent *TextComponent, UMaterialInterface *TextMaterial) -> void;

    // フォントサイズ変更処理
    auto UpdateAllTextActorSizes() -> void;
    auto UpdateAllTextActorOutlineWidth() -> void;

    // ユーティリティ関数
    static auto CombineActorTags(AActor *Actor) -> FString;
    static auto GetCameraLocation() -> FVector;

    // メンバ変数
    /** デバッグ描画デリゲートのハンドル */
    FDelegateHandle DrawDelegateHandle;

    /** テキストコンポーネント更新用のティッカーハンドル */
    FTSTicker::FDelegateHandle TickDelegateHandle;

    /** フォントサイズ変更デリゲートのハンドル */
    FDelegateHandle TextSizeChangedDelegateHandle;

    /** OutlineWidth変更デリゲートのハンドル */
    FDelegateHandle OutlineWidthChangedDelegateHandle;

    /** アクターごとのEditorActorTagDisplayActorを管理するマップ */
    TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AEditorActorTagDisplayActor>> TextActorMap;
};
