// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SpyGAS : ModuleRules
{
	public SpyGAS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
                "SpyGAS"
            });

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
				"CoreUObject",
				"Engine",
                "GameplayAbilities",
                "GameplayTags",
                "GameplayTasks"
            });
	}
}
