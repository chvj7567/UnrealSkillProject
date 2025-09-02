// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SkillProject : ModuleRules
{
	public SkillProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(new string[]
        {
            "SkillProject"
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput"
        });

		PrivateDependencyModuleNames.AddRange(new string[]
        {
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks"
        });
	}
}
