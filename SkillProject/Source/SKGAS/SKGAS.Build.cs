// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SKGAS : ModuleRules
{
	public SKGAS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
                "SKGAS"
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
