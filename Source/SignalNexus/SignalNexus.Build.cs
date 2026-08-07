// Copyright 2026 Silvan Teufel All Rights Reserved.

using UnrealBuildTool;

public class SignalNexus : ModuleRules
{
	public SignalNexus(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
		});
	}
}
