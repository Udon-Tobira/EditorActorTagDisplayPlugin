#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "Delegates/Delegate.h"
#include "EditorActorTagDisplaySettings.generated.h"

class AActor;

USTRUCT()
struct EDITORACTORTAGDISPLAY_API FActorClassTagDisplayConfig
{
    GENERATED_BODY() // NOLINT

    UPROPERTY(EditAnywhere, Category = "Actor Class Tag Display")
    TSoftClassPtr<AActor> ActorClass;

    UPROPERTY(EditAnywhere, Category = "Actor Class Tag Display")
    FLinearColor DisplayColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Actor Class Tag Display", meta = (DisplayName = "Position Offset"))
    FVector PositionOffset = FVector::ZeroVector;
};

UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Actor Tag Display"))
class EDITORACTORTAGDISPLAY_API UEditorActorTagDisplaySettings : public UDeveloperSettings
{
    GENERATED_BODY() // NOLINT

public:
    UEditorActorTagDisplaySettings();

    // UDeveloperSettings overrides
    auto GetContainerName() const -> FName override { return {"Editor"}; }
    auto GetCategoryName() const -> FName override { return {"Plugins"}; }
    auto GetSectionName() const -> FName override { return {"EditorActorTagDisplay"}; }

#if WITH_EDITOR
    void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // 設定値へのアクセサ
    auto GetClassConfigs() const -> const TArray<FActorClassTagDisplayConfig>& { return ClassConfigs; }
    auto IsTagDisplayEnabled() const -> bool { return bIsTagDisplayEnabled; }
    void SetTagDisplayEnabled(bool bEnabled);
    auto GetTextSize() const -> float { return TextSize; }
    void SetTextSize(float InTextSize);
    auto GetOutlineWidth() const -> float { return OutlineWidth; }
    void SetOutlineWidth(float InOutlineWidth);

    // 静的アクセサ
    static auto Get() -> UEditorActorTagDisplaySettings*;

    // デリゲート宣言
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnTextSizeChanged, float);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnOutlineWidthChanged, float);

    // デリゲートアクセサ（モジュール用）
    auto GetOnTextSizeChangedDelegate() -> FOnTextSizeChanged& { return OnTextSizeChanged; }
    auto GetOnOutlineWidthChangedDelegate() -> FOnOutlineWidthChanged& { return OnOutlineWidthChanged; }

private:
    // デリゲートインスタンス
    FOnTextSizeChanged OnTextSizeChanged;
    FOnOutlineWidthChanged OnOutlineWidthChanged;

    static constexpr float DefaultTextSize = 30.0F;
    static constexpr float DefaultOutlineWidth = 10.0F;

    UPROPERTY(config, EditAnywhere, Category = "Actor Tag Display", meta = (DisplayName = "Class Configurations"))
    TArray<FActorClassTagDisplayConfig> ClassConfigs;

    UPROPERTY(config, EditAnywhere, Category = "Actor Tag Display", meta = (DisplayName = "Enable Tag Display"))
    bool bIsTagDisplayEnabled = false;

    UPROPERTY(config, EditAnywhere, Category = "Actor Tag Display", meta = (DisplayName = "Text Size"))
    float TextSize = DefaultTextSize;

    UPROPERTY(config, EditAnywhere, Category = "Actor Tag Display", meta = (DisplayName = "Outline Width"))
    float OutlineWidth = DefaultOutlineWidth;
};
