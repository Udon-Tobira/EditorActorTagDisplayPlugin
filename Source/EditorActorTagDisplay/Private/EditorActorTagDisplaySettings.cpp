#include "EditorActorTagDisplaySettings.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

UEditorActorTagDisplaySettings::UEditorActorTagDisplaySettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("Actor Tag Display");
}

void UEditorActorTagDisplaySettings::SetTagDisplayEnabled(bool bEnabled)
{
    bIsTagDisplayEnabled = bEnabled;
    SaveConfig();
}

void UEditorActorTagDisplaySettings::SetTextSize(float InTextSize)
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

void UEditorActorTagDisplaySettings::SetOutlineWidth(float InOutlineWidth)
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

auto UEditorActorTagDisplaySettings::Get() -> UEditorActorTagDisplaySettings*
{
    return GetMutableDefault<UEditorActorTagDisplaySettings>();
}

#if WITH_EDITOR
void UEditorActorTagDisplaySettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
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
