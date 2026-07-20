// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SKUICore : ModuleRules
{
	public SKUICore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG",
				"SlateCore",
				"SKAssetCore",
			});
	}
}
