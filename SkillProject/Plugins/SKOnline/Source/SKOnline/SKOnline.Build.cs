// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SKOnline : ModuleRules
{
	public SKOnline(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
			});
	}
}
