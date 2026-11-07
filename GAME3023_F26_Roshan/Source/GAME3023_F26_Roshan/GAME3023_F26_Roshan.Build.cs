// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GAME3023_F26_Roshan : ModuleRules
{
	public GAME3023_F26_Roshan(ReadOnlyTargetRules Target) : base(Target)
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
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"GAME3023_F26_Roshan",
			"GAME3023_F26_Roshan/Variant_Platforming",
			"GAME3023_F26_Roshan/Variant_Platforming/Animation",
			"GAME3023_F26_Roshan/Variant_Combat",
			"GAME3023_F26_Roshan/Variant_Combat/AI",
			"GAME3023_F26_Roshan/Variant_Combat/Animation",
			"GAME3023_F26_Roshan/Variant_Combat/Gameplay",
			"GAME3023_F26_Roshan/Variant_Combat/Interfaces",
			"GAME3023_F26_Roshan/Variant_Combat/UI",
			"GAME3023_F26_Roshan/Variant_SideScrolling",
			"GAME3023_F26_Roshan/Variant_SideScrolling/AI",
			"GAME3023_F26_Roshan/Variant_SideScrolling/Gameplay",
			"GAME3023_F26_Roshan/Variant_SideScrolling/Interfaces",
			"GAME3023_F26_Roshan/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
