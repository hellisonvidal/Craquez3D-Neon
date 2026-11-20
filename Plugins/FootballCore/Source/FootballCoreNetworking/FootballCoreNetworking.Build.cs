// Copyright Craquez Studio. All Rights Reserved.
// FootballCore Plugin - Networking & Deterministic Simulation Module

using UnrealBuildTool;

public class FootballCoreNetworking : ModuleRules
{
	public FootballCoreNetworking(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"NetCore",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Chaos"
		});
	}
}
