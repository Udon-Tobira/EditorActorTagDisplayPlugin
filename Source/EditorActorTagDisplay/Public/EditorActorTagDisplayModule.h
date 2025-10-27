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
    static constexpr const TCHAR* TEXT_MATERIAL_PATH = TEXT("/EditorActorTagDisplay/Materials/M_TextMaterial.M_TextMaterial");

    // IModuleInterface implementation
    void StartupModule() override;
    void ShutdownModule() override;

private:
    // モジュール初期化・終了関連
    void RegisterDebugDrawDelegate();
    void UnregisterDebugDrawDelegate();
    static void AddViewportShowFlagExtension();
    void RemoveViewportShowFlagExtension();
    
    // テキストアクター管理
    void UpdateTextActors();
    void CleanupTextActors();
    void RemoveUnusedTextActors(const TSet<TWeakObjectPtr<AActor>>& ProcessedActors);
    
    // ワールド・アクター処理
    [[nodiscard]] static auto GetEditorWorld() -> UWorld*;
    void ProcessActorsInWorld(UWorld* World, const UEditorActorTagDisplaySettings* Settings, TSet<TWeakObjectPtr<AActor>>& ProcessedActors);
    void ProcessActorIfMatched(AActor* Actor, const UEditorActorTagDisplaySettings* Settings, TSet<TWeakObjectPtr<AActor>>& ProcessedActors);
    
    // テキストアクター作成・更新
    void CreateOrUpdateTextActor(AActor* Actor, const FActorClassTagDisplayConfig& Config);
    auto GetOrCreateTextActor(AActor* Actor) -> AEditorActorTagDisplayActor*;
    static void SetupTextActor(AEditorActorTagDisplayActor* TextActor);
    static void UpdateTextActorProperties(AEditorActorTagDisplayActor* TextActor, const FString& CombinedTags, const FActorClassTagDisplayConfig& Config, AActor* Actor);
    static void UpdateTextActorRotation(AEditorActorTagDisplayActor* TextActor, const FVector& TextPosition);
    
    // マテリアル設定
    static void SetTextMaterial(UTextRenderComponent* TextComponent);
    static void ApplyMaterial(UTextRenderComponent* TextComponent, UMaterialInterface* TextMaterial);
    
    // フォントサイズ変更処理
    void UpdateAllTextActorSizes();
    void UpdateAllTextActorOutlineWidth();
    
    // ユーティリティ関数
    static auto CombineActorTags(AActor* Actor) -> FString;
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
