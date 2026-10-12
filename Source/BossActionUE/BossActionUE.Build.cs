// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BossActionUE : ModuleRules
{
	public BossActionUE(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"BossActionUE",
			"BossActionUE/Variant_Platforming",
			"BossActionUE/Variant_Platforming/Animation",
			"BossActionUE/Variant_Combat",
			"BossActionUE/Variant_Combat/AI",
			"BossActionUE/Variant_Combat/Animation",
			"BossActionUE/Variant_Combat/Gameplay",
			"BossActionUE/Variant_Combat/Interfaces",
			"BossActionUE/Variant_Combat/UI",
			"BossActionUE/Variant_SideScrolling",
			"BossActionUE/Variant_SideScrolling/AI",
			"BossActionUE/Variant_SideScrolling/Gameplay",
			"BossActionUE/Variant_SideScrolling/Interfaces",
			"BossActionUE/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
