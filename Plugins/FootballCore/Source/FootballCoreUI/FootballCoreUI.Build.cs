// Copyright Craquez Studio. All Rights Reserved.
// FootballCore Plugin - Mobile-First Neon Sports UI & HUD Module

using UnrealBuildTool;

public class FootballCoreUI : ModuleRules
{
	public FootballCoreUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"FootballCoreNetworking",
			"FootballCoreGameplay",
			"FootballCoreAI"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"RenderCore"
		});
	}
}
