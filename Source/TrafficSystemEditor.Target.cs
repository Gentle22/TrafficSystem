// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class TrafficSystemEditorTarget : TargetRules
{
	public TrafficSystemEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "TrafficSystem" } );
		ExtraModuleNames.AddRange(new string[] { "TrafficSystemEditor" });
	}
}
