#include "EditorActorTagDisplaySettings.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

UEditorActorTagDisplaySettings::UEditorActorTagDisplaySettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("Actor Tag Display");
}

auto UEditorActorTagDisplaySettings::SetTagDisplayEnabled(bool bEnabled) -> void
{
    bIsTagDisplayEnabled = bEnabled;
    SaveConfig();
}

auto UEditorActorTagDisplaySettings::SetTextSize(float InTextSize) -> void
{
    if (FMath::IsNearlyEqual(TextSize, InTextSize, KINDA_SMALL_NUMBER))
    {
        return; // 値が変わっていない場合は何もしない
    }

    TextSize = InTextSize;
    SaveConfig();

    // デリゲートを呼び出してTextActorを更新
    OnTextSizeChanged.Broadcast(TextSize);
}

auto UEditorActorTagDisplaySettings::SetOutlineWidth(float InOutlineWidth) -> void
{
    if (FMath::IsNearlyEqual(OutlineWidth, InOutlineWidth, KINDA_SMALL_NUMBER))
    {
        return; // 値が変わっていない場合は何もしない
    }

    OutlineWidth = InOutlineWidth;
    SaveConfig();

    // デリゲートを呼び出してTextActorを更新
    OnOutlineWidthChanged.Broadcast(OutlineWidth);
}

auto UEditorActorTagDisplaySettings::Get() -> UEditorActorTagDisplaySettings *
{
    return GetMutableDefault<UEditorActorTagDisplaySettings>();
}

#if WITH_EDITOR
auto UEditorActorTagDisplaySettings::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property != nullptr)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();

        // TextSizeプロパティが変更された場合、デリゲートを呼び出す
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UEditorActorTagDisplaySettings, TextSize))
        {
            OnTextSizeChanged.Broadcast(TextSize);
        }
        // OutlineWidthプロパティが変更された場合、デリゲートを呼び出す
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UEditorActorTagDisplaySettings, OutlineWidth))
        {
            OnOutlineWidthChanged.Broadcast(OutlineWidth);
        }
    }
}
#endif
