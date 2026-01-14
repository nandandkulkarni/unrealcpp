// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPCPP : ModuleRules
{
	public TPCPP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"CinematicCamera",
			"Json",
			"JsonUtilities",
			"PCG",
			"Landscape"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"TPCPP",
			"TPCPP/Variant_Platforming",
			"TPCPP/Variant_Platforming/Animation",
			"TPCPP/Variant_Combat",
			"TPCPP/Variant_Combat/AI",
			"TPCPP/Variant_Combat/Animation",
			"TPCPP/Variant_Combat/Gameplay",
			"TPCPP/Variant_Combat/Interfaces",
			"TPCPP/Variant_Combat/UI",
			"TPCPP/Variant_SideScrolling",
			"TPCPP/Variant_SideScrolling/AI",
			"TPCPP/Variant_SideScrolling/Gameplay",
			"TPCPP/Variant_SideScrolling/Interfaces",
			"TPCPP/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
