// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SKAssetCore : ModuleRules
{
	public SKAssetCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			});
	}
}
