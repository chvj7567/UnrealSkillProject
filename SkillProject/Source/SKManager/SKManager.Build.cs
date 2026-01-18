// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SKManager : ModuleRules
{
	public SKManager(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
                "SKManager"
            });

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
				"CoreUObject",
				"Engine",
				"UMG",
            });
	}
}
