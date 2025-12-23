using UnrealBuildTool;

public class EditorActorTagDisplay : ModuleRules
{
    public EditorActorTagDisplay(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "UnrealEd",
            "LevelEditor",
            "Slate",
            "SlateCore",
            "DeveloperSettings",
            "ToolMenus"
        });
    }
}
