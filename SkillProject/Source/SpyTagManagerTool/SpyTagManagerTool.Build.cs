using UnrealBuildTool;

public class SpyTagManagerTool : ModuleRules
{
    public SpyTagManagerTool(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate", "SlateCore", "InputCore",
            "UnrealEd", "ToolMenus", "EditorFramework"
        });
    }
}
