// Copyright Craquez Studio. All Rights Reserved.
// FootballCore Plugin - Gameplay, Chaos Ball Aerodynamics, Procedural Animation, Stadium & Weather Module

using UnrealBuildTool;

public class FootballCoreGameplay : ModuleRules
{
	public FootballCoreGameplay(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"PhysicsCore",
			"Chaos",
			"ProceduralMeshComponent",
			"MotionWarping",
			"Niagara",
			"GameplayTags",
			"FootballCoreNetworking"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"RenderCore",
			"RHI",
			"ClothingSystemRuntimeInterface",
			"ClothingSystemRuntimeCommon"
		});
	}
}
