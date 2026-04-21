// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SkillProjectEditorTarget : TargetRules
{
	public SkillProjectEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("SkillProject");
        ExtraModuleNames.Add("SKGAS");
        ExtraModuleNames.Add("SpyDataEditorTool");
    }
}
