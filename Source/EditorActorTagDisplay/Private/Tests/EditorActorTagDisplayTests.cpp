// Copyright 2026 Udon-Tobira. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "EditorActorTagDisplayTestActor.h"

#include "EditorActorTagDisplayRuleMatcher.h"
#include "EditorActorTagDisplaySettings.h"
#include "EditorActorTagDisplayTemplateFormatter.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Tests/AutomationEditorCommon.h"
#include "Misc/AutomationTest.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "WorldPartition/DataLayer/DataLayerInstance.h"
#include "WorldPartition/DataLayer/DataLayerInstanceWithAsset.h"
#include "WorldPartition/DataLayer/WorldDataLayers.h"

namespace
{
    AEditorActorTagDisplayTestActor* MakeTestActor()
    {
        return NewObject<AEditorActorTagDisplayTestActor>(GetTransientPackage(), NAME_None, RF_Transient);
    }

    FResolvedActorMetadataOverlayRule MakeRule(UClass* ActorClass)
    {
        FResolvedActorMetadataOverlayRule Rule;
        Rule.Rule.bEnabled = true;
        Rule.Rule.ActorClass = ActorClass;
        Rule.ResolvedActorClass = ActorClass;
        return Rule;
    }

    int32 FindWithSingleRule(const AActor& Actor, const FResolvedActorMetadataOverlayRule& Rule)
    {
        TArray<FResolvedActorMetadataOverlayRule> Rules;
        Rules.Add(Rule);
        return EditorActorTagDisplayRuleMatcher::FindMatchingRule(Actor, Rules);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEditorActorTagDisplayRuleMatchingTest,
                                 "EditorActorTagDisplay.RuleMatching",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEditorActorTagDisplayRuleMatchingTest::RunTest(const FString& Parameters)
{
    AEditorActorTagDisplayTestActor* Actor = MakeTestActor();
    Actor->Tags = {FName(TEXT("Required")), FName(TEXT("Other"))};

    FResolvedActorMetadataOverlayRule DisabledRule = MakeRule(AEditorActorTagDisplayTestActor::StaticClass());
    DisabledRule.Rule.bEnabled = false;
    TestEqual(TEXT("Disabled only does not match"), FindWithSingleRule(*Actor, DisabledRule), INDEX_NONE);

    FResolvedActorMetadataOverlayRule ExactClassRule = MakeRule(AEditorActorTagDisplayTestActor::StaticClass());
    ExactClassRule.Rule.bIncludeDerivedClasses = false;
    TestEqual(TEXT("Exact class matches the exact actor class"), FindWithSingleRule(*Actor, ExactClassRule), 0);

    FResolvedActorMetadataOverlayRule ExactBaseClassRule = MakeRule(AActor::StaticClass());
    ExactBaseClassRule.Rule.bIncludeDerivedClasses = false;
    TestEqual(TEXT("Exact class rejects a derived actor"), FindWithSingleRule(*Actor, ExactBaseClassRule), INDEX_NONE);

    FResolvedActorMetadataOverlayRule IncludeDerivedRule = MakeRule(AActor::StaticClass());
    IncludeDerivedRule.Rule.bIncludeDerivedClasses = true;
    TestEqual(TEXT("Include derived accepts a derived actor"), FindWithSingleRule(*Actor, IncludeDerivedRule), 0);

    FResolvedActorMetadataOverlayRule MissingRequiredTagRule = MakeRule(AEditorActorTagDisplayTestActor::StaticClass());
    MissingRequiredTagRule.Rule.RequiredActorTags = {FName(TEXT("Missing"))};
    TestEqual(TEXT("Missing required tag rejects the rule"), FindWithSingleRule(*Actor, MissingRequiredTagRule), INDEX_NONE);

    FResolvedActorMetadataOverlayRule ExcludedTagRule = MakeRule(AEditorActorTagDisplayTestActor::StaticClass());
    ExcludedTagRule.Rule.ExcludedActorTags = {FName(TEXT("Other"))};
    TestEqual(TEXT("Present excluded tag rejects the rule"), FindWithSingleRule(*Actor, ExcludedTagRule), INDEX_NONE);

    TArray<FResolvedActorMetadataOverlayRule> TwoMatchingRules;
    TwoMatchingRules.Add(MakeRule(AEditorActorTagDisplayTestActor::StaticClass()));
    TwoMatchingRules.Add(MakeRule(AActor::StaticClass()));
    TestEqual(TEXT("The first of two matching rules wins"), EditorActorTagDisplayRuleMatcher::FindMatchingRule(*Actor, TwoMatchingRules), 0);

    FResolvedActorMetadataOverlayRule NullClassRule = MakeRule(nullptr);
    NullClassRule.Rule.ActorClass.Reset();
    NullClassRule.ResolvedActorClass = nullptr;
    TestEqual(TEXT("Null class never matches"), FindWithSingleRule(*Actor, NullClassRule), INDEX_NONE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEditorActorTagDisplayFixedTemplateTokensTest,
                                 "EditorActorTagDisplay.FixedTemplateTokens",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEditorActorTagDisplayFixedTemplateTokensTest::RunTest(const FString& Parameters)
{
    UWorld* EditorWorld = FAutomationEditorCommonUtils::CreateNewMap();
    if (!TestNotNull(TEXT("CreateNewMap returns an editor world"), EditorWorld))
    {
        return false;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Name = FName(TEXT("MetadataOverlayTestActor"));
    SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ReturnNull;
    AEditorActorTagDisplayTestActor* Actor = EditorWorld->SpawnActor<AEditorActorTagDisplayTestActor>(
        AEditorActorTagDisplayTestActor::StaticClass(), FTransform::Identity, SpawnParameters);
    if (!TestNotNull(TEXT("Spawn the metadata overlay test actor in the editor world"), Actor))
    {
        return false;
    }

    Actor->SetActorLabel(TEXT("Display Label"));
    Actor->Tags = {FName(TEXT("zeta")), FName(TEXT("Alpha"))};
    const FGameplayTag AlphaTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("EditorActorTagDisplay.Test.Alpha")),
        false);

    const FGameplayTag ZetaTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("EditorActorTagDisplay.Test.Zeta")),
        false);

    if (!TestTrue(TEXT("Alpha automation tag is loaded from the host project config"), AlphaTag.IsValid()) ||
        !TestTrue(TEXT("Zeta automation tag is loaded from the host project config"), ZetaTag.IsValid()))
    {
        return false;
    }

    Actor->GameplayTags.AddTag(ZetaTag);
    Actor->GameplayTags.AddTag(AlphaTag);
    Actor->SetFolderPath(FName(TEXT("OverlayTests/Folder")));

    TSet<FString> Warnings;
    const FString Template = TEXT("{ActorLabel}\n{ActorName}\n{ActorClass}\n{ActorTags}\n{GameplayTags}\n{Folder}\n{DataLayers}");
    const FString Expected = TEXT("Display Label\nMetadataOverlayTestActor\nEditorActorTagDisplayTestActor\nAlpha, zeta\nEditorActorTagDisplay.Test.Alpha, EditorActorTagDisplay.Test.Zeta\nOverlayTests/Folder\n");
    const FString Output = FEditorActorTagDisplayTemplateFormatter::Format(*Actor, Template, 120, Warnings);
    TestEqual(TEXT("Fixed tokens render the complete editor-world output"), Output, Expected);

    EditorWorld->DestroyActor(Actor);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEditorActorTagDisplayDataLayerFormattingTest,
                                 "EditorActorTagDisplay.DataLayerFormatting",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEditorActorTagDisplayDataLayerFormattingTest::RunTest(const FString& Parameters)
{
    GEditor->CreateNewMapForEditing(false, true);
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!TestNotNull(TEXT("Create a temporary editor world"), EditorWorld) ||
        !TestTrue(TEXT("The temporary editor world uses World Partition"), EditorWorld->IsPartitionedWorld()))
    {
        return false;
    }

    UDataLayerAsset* GameplayAsset = NewObject<UDataLayerAsset>(
        CreatePackage(TEXT("/Temp/EditorActorTagDisplayTests/Gameplay")), TEXT("Gameplay"), RF_Transient);
    UDataLayerAsset* NightAsset = NewObject<UDataLayerAsset>(
        CreatePackage(TEXT("/Temp/EditorActorTagDisplayTests/Night")), TEXT("Night"), RF_Transient);
    if (!TestNotNull(TEXT("Create the Gameplay Data Layer asset"), GameplayAsset) ||
        !TestNotNull(TEXT("Create the Night Data Layer asset"), NightAsset))
    {
        return false;
    }

    GameplayAsset->SetType(EDataLayerType::Editor);
    NightAsset->SetType(EDataLayerType::Editor);
    GameplayAsset->OnCreated();
    NightAsset->OnCreated();

    AWorldDataLayers* WorldDataLayers = EditorWorld->GetWorldDataLayers();
    if (!TestNotNull(TEXT("Get the World Partition Data Layers actor"), WorldDataLayers))
    {
        return false;
    }

    UDataLayerInstance* GameplayInstance =
        WorldDataLayers->CreateDataLayer<UDataLayerInstanceWithAsset>(GameplayAsset);
    UDataLayerInstance* NightInstance =
        WorldDataLayers->CreateDataLayer<UDataLayerInstanceWithAsset>(NightAsset);
    if (!TestNotNull(TEXT("Create the Gameplay Data Layer instance"), GameplayInstance) ||
        !TestNotNull(TEXT("Create the Night Data Layer instance"), NightInstance))
    {
        return false;
    }

    AEditorActorTagDisplayTestActor* LayeredActor = EditorWorld->SpawnActor<AEditorActorTagDisplayTestActor>();
    AEditorActorTagDisplayTestActor* UnlayeredActor = EditorWorld->SpawnActor<AEditorActorTagDisplayTestActor>();
    if (!TestNotNull(TEXT("Spawn the layered test actor"), LayeredActor) ||
        !TestNotNull(TEXT("Spawn the unlayered test actor"), UnlayeredActor))
    {
        return false;
    }

    LayeredActor->SetPackageExternal(true, false);
    UnlayeredActor->SetPackageExternal(true, false);
    if (!TestTrue(TEXT("The layered actor uses an external actor package"), LayeredActor->IsPackageExternal()) ||
        !TestTrue(TEXT("The unlayered actor uses an external actor package"), UnlayeredActor->IsPackageExternal()))
    {
        return false;
    }

    TestTrue(TEXT("Assign Night before Gameplay to exercise stable sorting"),
             LayeredActor->AddDataLayer(NightInstance));
    TestTrue(TEXT("Assign Gameplay after Night"),
             LayeredActor->AddDataLayer(GameplayInstance));
    LayeredActor->AddDataLayer(GameplayInstance);

    TSet<FString> Warnings;
    const FString LayeredOutput = FEditorActorTagDisplayTemplateFormatter::Format(
        *LayeredActor, TEXT("{DataLayers}"), 120, Warnings);
    TestEqual(TEXT("Data Layers use sorted, deduplicated asset names"), LayeredOutput, FString(TEXT("Gameplay, Night")));
    TestFalse(TEXT("Data Layers never expose generated internal identifiers"), LayeredOutput.Contains(TEXT("DataLayer_")));

    const FString UnlayeredOutput = FEditorActorTagDisplayTemplateFormatter::Format(
        *UnlayeredActor, TEXT("{DataLayers}"), 120, Warnings);
    TestTrue(TEXT("An actor without Data Layers formats as an empty string"), UnlayeredOutput.IsEmpty());

    EditorWorld->DestroyActor(LayeredActor);
    EditorWorld->DestroyActor(UnlayeredActor);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEditorActorTagDisplayTemplateValidationTest,
                                 "EditorActorTagDisplay.TemplateValidation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEditorActorTagDisplayTemplateValidationTest::RunTest(const FString& Parameters)
{
    AEditorActorTagDisplayTestActor* Actor = MakeTestActor();
    TSet<FString> Warnings;
    const FString Output = FEditorActorTagDisplayTemplateFormatter::Format(
        *Actor, TEXT("{UnknownToken}|{Property:Missing}|{Property:TestString.Value}|literal {"), 120, Warnings);
    const FString Expected = TEXT("<unknown:UnknownToken>|<missing:Missing>|<unsupported:TestString.Value>|literal {");
    TestEqual(TEXT("Invalid template tokens produce the documented complete output"), Output, Expected);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEditorActorTagDisplayPropertyFormattingTest,
                                 "EditorActorTagDisplay.PropertyFormatting",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEditorActorTagDisplayPropertyFormattingTest::RunTest(const FString& Parameters)
{
    AEditorActorTagDisplayTestActor* Actor = MakeTestActor();
    Actor->ObjectReference = nullptr;
    Actor->SoftObjectReference.Reset();
    Actor->TestString = TEXT("Line one\nLine two\twith tab");
    TSet<FString> Warnings;

    const auto FormatProperty = [&Actor, &Warnings](const TCHAR* PropertyName)
    {
        return FEditorActorTagDisplayTemplateFormatter::Format(
            *Actor, FString::Printf(TEXT("{Property:%s}"), PropertyName), 120, Warnings);
    };

    TestEqual(TEXT("Boolean formatting is stable"), FormatProperty(TEXT("bTestBool")), FString(TEXT("true")));
    TestEqual(TEXT("Signed integer formatting is stable"), FormatProperty(TEXT("TestInteger")), FString(TEXT("42")));
    TestEqual(TEXT("Unsigned integer formatting is stable"), FormatProperty(TEXT("TestUnsignedInteger")), FString(TEXT("84")));
    TestEqual(TEXT("Float formatting is stable"), FormatProperty(TEXT("TestFloat")), FString(TEXT("1.5")));
    TestEqual(TEXT("Double formatting is stable"), FormatProperty(TEXT("TestDouble")), FString(TEXT("2.5")));
    TestEqual(TEXT("FName formatting is stable"), FormatProperty(TEXT("TestName")), FString(TEXT("TestName")));
    TestEqual(TEXT("FString line breaks and tabs are normalized"), FormatProperty(TEXT("TestString")), FString(TEXT("Line one Line two with tab")));
    TestEqual(TEXT("FText formatting is stable"), FormatProperty(TEXT("TestText")), FString(TEXT("Test Text")));
    TestEqual(TEXT("Enum formatting is stable"), FormatProperty(TEXT("TestEnum")), FString(TEXT("Second")));
    TestEqual(TEXT("Null UObject references are explicit"), FormatProperty(TEXT("ObjectReference")), FString(TEXT("None")));
    TestEqual(TEXT("Null soft object references are explicit"), FormatProperty(TEXT("SoftObjectReference")), FString(TEXT("None")));
    TestTrue(TEXT("Struct formatting contains X"), FormatProperty(TEXT("TestStruct")).Contains(TEXT("X=1")));
    TestTrue(TEXT("Struct formatting contains Y"), FormatProperty(TEXT("TestStruct")).Contains(TEXT("Y=2")));
    TestTrue(TEXT("Struct formatting contains Z"), FormatProperty(TEXT("TestStruct")).Contains(TEXT("Z=3")));
    TestEqual(TEXT("Non-public properties are rejected"), FormatProperty(TEXT("PrivateProperty")), FString(TEXT("<unsupported:PrivateProperty>")));
    TestEqual(TEXT("Transient properties are rejected"), FormatProperty(TEXT("TransientString")), FString(TEXT("<unsupported:TransientString>")));

    const FString ShortOutput = FormatProperty(TEXT("TestString"));
    const FString TruncatedOutput = FEditorActorTagDisplayTemplateFormatter::Format(*Actor, TEXT("{Property:TestString}"), 16, Warnings);
    TestTrue(TEXT("Max length output is at most 16 characters"), TruncatedOutput.Len() <= 16);
    TestTrue(TEXT("Max length output ends with an ellipsis"), TruncatedOutput.EndsWith(TEXT("...")));
    TestFalse(TEXT("The full normalized string is not confused with the shortened output"), ShortOutput == TruncatedOutput);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEditorActorTagDisplayDefaultConfigurationTest,
                                 "EditorActorTagDisplay.DefaultConfiguration",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEditorActorTagDisplayDefaultConfigurationTest::RunTest(const FString& Parameters)
{
    const UEditorActorTagDisplayUserSettings* UserSettings = GetDefault<UEditorActorTagDisplayUserSettings>();
    const UEditorActorTagDisplayProjectSettings* ProjectSettings = GetDefault<UEditorActorTagDisplayProjectSettings>();
    TestEqual(TEXT("Default display mode is Selected"), UserSettings->DisplayMode, EActorMetadataOverlayMode::Selected);
    TestEqual(TEXT("Default text scale is one"), UserSettings->TextScale, 1.0f);
    TestEqual(TEXT("Default distance is 10000"), UserSettings->GlobalMaxDrawDistance, 10000.0f);
    TestEqual(TEXT("Default project template is stable"), ProjectSettings->DefaultDisplayTemplate, FString(TEXT("{ActorLabel}\nClass: {ActorClass}\nTags: {ActorTags}")));
    TestEqual(TEXT("One default rule is present"), ProjectSettings->Rules.Num(), 1);
    if (ProjectSettings->Rules.Num() != 1)
    {
        return false;
    }

    TestEqual(TEXT("Default rule targets Actor"), ProjectSettings->Rules[0].ActorClass.Get(), AActor::StaticClass());
    return true;
}

#endif
