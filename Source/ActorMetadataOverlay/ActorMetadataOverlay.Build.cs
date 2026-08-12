// Copyright (c) 2026 metyatech. All rights reserved.

using UnrealBuildTool;

public class ActorMetadataOverlay : ModuleRules
{
    public ActorMetadataOverlay(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        PrecompileForTargets = PrecompileTargetsType.Editor;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "UnrealEd",
            "LevelEditor",
            "Slate",
            "SlateCore",
            "DeveloperSettings",
            "ToolMenus",
            "Settings"
        });
    }
}
