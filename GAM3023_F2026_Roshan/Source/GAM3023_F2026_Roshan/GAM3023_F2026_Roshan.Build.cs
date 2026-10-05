// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GAM3023_F2026_Roshan : ModuleRules
{
	public GAM3023_F2026_Roshan(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"GAM3023_F2026_Roshan",
			"GAM3023_F2026_Roshan/Variant_Strategy",
			"GAM3023_F2026_Roshan/Variant_Strategy/UI",
			"GAM3023_F2026_Roshan/Variant_TwinStick",
			"GAM3023_F2026_Roshan/Variant_TwinStick/AI",
			"GAM3023_F2026_Roshan/Variant_TwinStick/Gameplay",
			"GAM3023_F2026_Roshan/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
