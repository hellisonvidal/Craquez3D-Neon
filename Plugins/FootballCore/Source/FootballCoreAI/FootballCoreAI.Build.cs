// Copyright Craquez Studio. All Rights Reserved.
// FootballCore Plugin - Tactical AI Module (StateTree, BehaviorTree, GOAP & RL-Ready Bridge)

using UnrealBuildTool;

public class FootballCoreAI : ModuleRules
{
	public FootballCoreAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"GameplayTags",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"FootballCoreNetworking",
			"FootballCoreGameplay"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine"
		});
	}
}
