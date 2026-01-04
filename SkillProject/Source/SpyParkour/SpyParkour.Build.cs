// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SpyParkour : ModuleRules
{
	public SpyParkour(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
                "SpyParkour"
            });

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
				"CoreUObject",
				"Engine"
            });
	}
}
