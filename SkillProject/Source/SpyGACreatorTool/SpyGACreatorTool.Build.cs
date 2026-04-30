using UnrealBuildTool;

public class SpyGACreatorTool : ModuleRules
{
    public SpyGACreatorTool(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine",
            "GameplayAbilities", "GameplayTags"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate", "SlateCore", "InputCore",
            "UnrealEd", "Kismet",
            "GameplayTagsEditor", "EditorWidgets",
            "ToolMenus", "EditorFramework",
            "AssetRegistry", "PropertyEditor"
        });
    }
}
