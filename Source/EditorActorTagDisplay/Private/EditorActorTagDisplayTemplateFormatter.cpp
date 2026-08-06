// Copyright 2026 Udon-Tobira. All Rights Reserved.

#include "EditorActorTagDisplayTemplateFormatter.h"

#include "EditorActorTagDisplayLog.h"
#include "GameFramework/Actor.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "UObject/EnumProperty.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UnrealType.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "WorldPartition/DataLayer/DataLayerInstance.h"

namespace
{
    void SortCaseInsensitive(TArray<FString>& Values)
    {
        Values.Sort([](const FString& Left, const FString& Right)
        {
            return FCString::Stricmp(*Left, *Right) < 0;
        });
    }

    void SortDataLayerNames(TArray<FString>& Values)
    {
        Values.Sort([](const FString& Left, const FString& Right)
        {
            const int32 IgnoreCaseComparison = FCString::Stricmp(*Left, *Right);
            return IgnoreCaseComparison != 0
                       ? IgnoreCaseComparison < 0
                       : FCString::Strcmp(*Left, *Right) < 0;
        });
    }

    bool ContainsGeneratedDataLayerName(const FString& Value)
    {
        constexpr int32 PrefixLength = 10;
        constexpr int32 GuidLength = 32;
        const FString Prefix(TEXT("DataLayer_"));
        int32 SearchStart = 0;
        while (SearchStart < Value.Len())
        {
            const int32 PrefixIndex = Value.Find(
                Prefix, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchStart);
            if (PrefixIndex == INDEX_NONE)
            {
                return false;
            }

            const int32 GuidStart = PrefixIndex + PrefixLength;
            const int32 GuidEnd = GuidStart + GuidLength;
            if (GuidEnd <= Value.Len())
            {
                bool bAllHexDigits = true;
                for (int32 Index = GuidStart; Index < GuidEnd; ++Index)
                {
                    if (!FChar::IsHexDigit(Value[Index]))
                    {
                        bAllHexDigits = false;
                        break;
                    }
                }

                const bool bEndsAtBoundary = GuidEnd == Value.Len() ||
                                             !FChar::IsAlnum(Value[GuidEnd]);
                if (bAllHexDigits && bEndsAtBoundary)
                {
                    return true;
                }
            }

            SearchStart = PrefixIndex + PrefixLength;
        }

        return false;
    }

    bool IsUsableDataLayerDisplayName(const FString& Value)
    {
        return !Value.IsEmpty() &&
               Value != TEXT("Invalid Data Layer") &&
               !ContainsGeneratedDataLayerName(Value);
    }

    FString GetDataLayerDisplayName(const UDataLayerInstance& DataLayerInstance)
    {
        if (const UDataLayerAsset* DataLayerAsset = DataLayerInstance.GetAsset())
        {
            FString AssetName = DataLayerAsset->GetName();
            AssetName.TrimStartAndEndInline();
            if (IsUsableDataLayerDisplayName(AssetName))
            {
                return AssetName;
            }
        }

        FString ShortName = DataLayerInstance.GetDataLayerShortName();
        ShortName.TrimStartAndEndInline();
        if (IsUsableDataLayerDisplayName(ShortName))
        {
            return ShortName;
        }

        FString FullName = DataLayerInstance.GetDataLayerFullName();
        FullName.TrimStartAndEndInline();
        return IsUsableDataLayerDisplayName(FullName) ? FullName : FString();
    }

    FString JoinActorTags(const AActor& Actor)
    {
        TArray<FString> Values;
        Values.Reserve(Actor.Tags.Num());
        for (const FName& Tag : Actor.Tags)
        {
            Values.Add(Tag.ToString());
        }
        SortCaseInsensitive(Values);
        return FString::Join(Values, TEXT(", "));
    }

    FString JoinGameplayTags(const AActor& Actor)
    {
        const IGameplayTagAssetInterface* GameplayTagInterface = Cast<IGameplayTagAssetInterface>(&Actor);
        if (GameplayTagInterface == nullptr)
        {
            return FString();
        }

        FGameplayTagContainer OwnedTags;
        GameplayTagInterface->GetOwnedGameplayTags(OwnedTags);

        TArray<FString> Values;
        for (const FGameplayTag& Tag : OwnedTags)
        {
            Values.Add(Tag.ToString());
        }
        SortCaseInsensitive(Values);
        return FString::Join(Values, TEXT(", "));
    }

    FString JoinDataLayers(const AActor& Actor)
    {
        TArray<FString> Values;
        for (const UDataLayerInstance* DataLayerInstance : Actor.GetDataLayerInstances())
        {
            if (DataLayerInstance == nullptr)
            {
                continue;
            }

            FString DisplayName = GetDataLayerDisplayName(*DataLayerInstance);
            if (DisplayName.IsEmpty())
            {
                continue;
            }

            FString* ExistingValue = Values.FindByPredicate([&DisplayName](const FString& Value)
            {
                return Value.Equals(DisplayName, ESearchCase::IgnoreCase);
            });
            if (ExistingValue != nullptr)
            {
                if (FCString::Strcmp(*DisplayName, **ExistingValue) < 0)
                {
                    *ExistingValue = MoveTemp(DisplayName);
                }
                continue;
            }

            Values.Add(MoveTemp(DisplayName));
        }
        SortDataLayerNames(Values);
        return FString::Join(Values, TEXT(", "));
    }

    bool IsUnsignedIntegerProperty(const FProperty* Property)
    {
        return CastField<FByteProperty>(Property) != nullptr ||
               CastField<FUInt16Property>(Property) != nullptr ||
               CastField<FUInt32Property>(Property) != nullptr ||
               CastField<FUInt64Property>(Property) != nullptr;
    }

    FString FormatFloatingPoint(double Value)
    {
        FString Formatted = LexToString(Value);
        const int32 DecimalIndex = Formatted.Find(TEXT("."));
        if (DecimalIndex == INDEX_NONE)
        {
            return Formatted;
        }

        int32 MantissaEnd = Formatted.Len();
        for (int32 Index = DecimalIndex + 1; Index < Formatted.Len(); ++Index)
        {
            if (Formatted[Index] == TCHAR('e') || Formatted[Index] == TCHAR('E'))
            {
                MantissaEnd = Index;
                break;
            }
        }

        int32 TrimmedEnd = MantissaEnd;
        while (TrimmedEnd > DecimalIndex + 1 && Formatted[TrimmedEnd - 1] == TCHAR('0'))
        {
            --TrimmedEnd;
        }
        if (TrimmedEnd == DecimalIndex + 1)
        {
            --TrimmedEnd;
        }

        if (TrimmedEnd == MantissaEnd)
        {
            return Formatted;
        }

        return Formatted.Left(TrimmedEnd) + Formatted.Mid(MantissaEnd);
    }

    bool IsDirectPropertyName(const FString& PropertyName)
    {
        return !PropertyName.IsEmpty() &&
               !PropertyName.Contains(TEXT(".")) &&
               !PropertyName.Contains(TEXT("[")) &&
               !PropertyName.Contains(TEXT("]")) &&
               !PropertyName.Contains(TEXT("(")) &&
               !PropertyName.Contains(TEXT(")"));
    }

    void NormalizePropertyValue(FString& Value, int32 MaxPropertyValueLength)
    {
        Value.ReplaceInline(TEXT("\r\n"), TEXT(" "));
        Value.ReplaceInline(TEXT("\r"), TEXT(" "));
        Value.ReplaceInline(TEXT("\n"), TEXT(" "));
        Value.ReplaceInline(TEXT("\t"), TEXT(" "));
        Value.TrimStartAndEndInline();

        const int32 SafeMaxLength = FMath::Max(1, MaxPropertyValueLength);
        if (Value.Len() > SafeMaxLength)
        {
            if (SafeMaxLength <= 3)
            {
                Value = FString::ChrN(SafeMaxLength, TEXT('.'));
            }
            else
            {
                Value = Value.Left(SafeMaxLength - 3) + TEXT("...");
            }
        }
    }

    void WarnAboutProperty(const AActor& Actor,
                           const FString& PropertyName,
                           const TCHAR* Reason,
                           TSet<FString>& InOutWarnedPropertyKeys)
    {
        const FString WarningKey = Actor.GetClass()->GetPathName() + TEXT("::") + PropertyName;
        if (InOutWarnedPropertyKeys.Contains(WarningKey))
        {
            return;
        }

        InOutWarnedPropertyKeys.Add(WarningKey);
        UE_LOG(LogEditorActorTagDisplay, Warning, TEXT("Property token '%s' on class '%s' is %s."),
               *PropertyName, *Actor.GetClass()->GetPathName(), Reason);
    }

    FString FormatEnumValue(const FEnumProperty* EnumProperty, const void* ValuePtr)
    {
        const FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
        const int64 Value = UnderlyingProperty->GetSignedIntPropertyValue(ValuePtr);
        const UEnum* Enum = EnumProperty->GetEnum();
        if (Enum == nullptr)
        {
            return LexToString(Value);
        }

        const FText DisplayName = Enum->GetDisplayNameTextByValue(Value);
        return DisplayName.IsEmpty() ? Enum->GetNameStringByValue(Value) : DisplayName.ToString();
    }

    FString FormatByteEnumValue(const FByteProperty* ByteProperty, const void* ValuePtr)
    {
        const uint8 Value = ByteProperty->GetPropertyValue(ValuePtr);
        const UEnum* Enum = ByteProperty->GetIntPropertyEnum();
        if (Enum == nullptr)
        {
            return LexToString(Value);
        }

        const FText DisplayName = Enum->GetDisplayNameTextByValue(Value);
        return DisplayName.IsEmpty() ? Enum->GetNameStringByValue(Value) : DisplayName.ToString();
    }
}

FString FEditorActorTagDisplayTemplateFormatter::Format(const AActor& Actor,
                                                        const FString& Template,
                                                        int32 MaxPropertyValueLength,
                                                        TSet<FString>& InOutWarnedPropertyKeys)
{
    FString Output;
    int32 SearchStart = 0;

    while (SearchStart < Template.Len())
    {
        const int32 OpenBrace = Template.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchStart);
        if (OpenBrace == INDEX_NONE)
        {
            Output += Template.Mid(SearchStart);
            break;
        }

        Output += Template.Mid(SearchStart, OpenBrace - SearchStart);
        const int32 CloseBrace = Template.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, OpenBrace + 1);
        if (CloseBrace == INDEX_NONE)
        {
            Output += Template.Mid(OpenBrace);
            break;
        }

        const FString Token = Template.Mid(OpenBrace + 1, CloseBrace - OpenBrace - 1);
        FString Replacement;
        if (Token == TEXT("ActorLabel"))
        {
            Replacement = Actor.GetActorLabel();
        }
        else if (Token == TEXT("ActorName"))
        {
            Replacement = Actor.GetName();
        }
        else if (Token == TEXT("ActorClass"))
        {
            Replacement = Actor.GetClass()->GetName();
            if (Replacement.EndsWith(TEXT("_C")))
            {
                Replacement.LeftChopInline(2);
            }
        }
        else if (Token == TEXT("ActorTags"))
        {
            Replacement = JoinActorTags(Actor);
        }
        else if (Token == TEXT("GameplayTags"))
        {
            Replacement = JoinGameplayTags(Actor);
        }
        else if (Token == TEXT("Folder"))
        {
            Replacement = Actor.GetWorld() ? Actor.GetFolderPath().ToString() : FString();
        }
        else if (Token == TEXT("DataLayers"))
        {
            Replacement = JoinDataLayers(Actor);
        }
        else if (Token.StartsWith(TEXT("Property:"), ESearchCase::CaseSensitive))
        {
            Replacement = FormatProperty(Actor, Token.RightChop(9), MaxPropertyValueLength, InOutWarnedPropertyKeys);
        }
        else
        {
            Replacement = TEXT("<unknown:") + Token + TEXT(">");
        }

        Output += Replacement;
        SearchStart = CloseBrace + 1;
    }

    return Output;
}

FString FEditorActorTagDisplayTemplateFormatter::FormatProperty(const AActor& Actor,
                                                                const FString& PropertyName,
                                                                int32 MaxPropertyValueLength,
                                                                TSet<FString>& InOutWarnedPropertyKeys)
{
    if (!IsDirectPropertyName(PropertyName))
    {
        WarnAboutProperty(Actor, PropertyName, TEXT("not a direct top-level property"), InOutWarnedPropertyKeys);
        return TEXT("<unsupported:") + PropertyName + TEXT(">");
    }

    FProperty* Property = FindFProperty<FProperty>(Actor.GetClass(), FName(*PropertyName));
    if (Property == nullptr)
    {
        WarnAboutProperty(Actor, PropertyName, TEXT("missing"), InOutWarnedPropertyKeys);
        return TEXT("<missing:") + PropertyName + TEXT(">");
    }

    if (!Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible) ||
        Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated))
    {
        WarnAboutProperty(Actor, PropertyName, TEXT("not an eligible public property"), InOutWarnedPropertyKeys);
        return TEXT("<unsupported:") + PropertyName + TEXT(">");
    }

    const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(&Actor);
    FString Value;

    if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        Value = BoolProperty->GetPropertyValue(ValuePtr) ? TEXT("true") : TEXT("false");
    }
    else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
    {
        Value = FormatEnumValue(EnumProperty, ValuePtr);
    }
    else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
    {
        Value = ByteProperty->GetIntPropertyEnum() != nullptr
                    ? FormatByteEnumValue(ByteProperty, ValuePtr)
                    : LexToString(ByteProperty->GetPropertyValue(ValuePtr));
    }
    else if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
    {
        if (NumericProperty->IsFloatingPoint())
        {
            Value = FormatFloatingPoint(NumericProperty->GetFloatingPointPropertyValue(ValuePtr));
        }
        else if (NumericProperty->IsInteger())
        {
            Value = IsUnsignedIntegerProperty(Property)
                        ? LexToString(NumericProperty->GetUnsignedIntPropertyValue(ValuePtr))
                        : LexToString(NumericProperty->GetSignedIntPropertyValue(ValuePtr));
        }
        else
        {
            WarnAboutProperty(Actor, PropertyName, TEXT("an unsupported numeric type"), InOutWarnedPropertyKeys);
            return TEXT("<unsupported:") + PropertyName + TEXT(">");
        }
    }
    else if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        Value = NameProperty->GetPropertyValue(ValuePtr).ToString();
    }
    else if (const FStrProperty* StringProperty = CastField<FStrProperty>(Property))
    {
        Value = StringProperty->GetPropertyValue(ValuePtr);
    }
    else if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
    {
        Value = TextProperty->GetPropertyValue(ValuePtr).ToString();
    }
    else if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
    {
        const FSoftObjectPtr& SoftObject = SoftObjectProperty->GetPropertyValue(ValuePtr);
        Value = SoftObject.IsNull() ? TEXT("None") : SoftObject.ToSoftObjectPath().ToString();
    }
    else if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
    {
        UObject* Object = ObjectProperty->GetObjectPropertyValue(ValuePtr);
        Value = Object == nullptr ? TEXT("None") : Object->GetPathName();
    }
    else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
    {
        StructProperty->ExportTextItem_Direct(Value, ValuePtr, nullptr, const_cast<AActor*>(&Actor), PPF_None);
    }
    else
    {
        WarnAboutProperty(Actor, PropertyName, TEXT("an unsupported property type"), InOutWarnedPropertyKeys);
        return TEXT("<unsupported:") + PropertyName + TEXT(">");
    }

    NormalizePropertyValue(Value, MaxPropertyValueLength);
    return Value;
}
