using UnrealBuildTool;

public class SpyDataEditorTool : ModuleRules
{
    public SpyDataEditorTool(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "SkillProject", "GameplayTags", "SKAssetCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UnrealEd", "PropertyEditor", "AssetRegistry", "GameplayTagsEditor",
            "Slate", "SlateCore", "InputCore",
            "ToolMenus", "EditorFramework", "EditorWidgets",
            "EditorSubsystem", "AssetTools"
        });
    }
}
