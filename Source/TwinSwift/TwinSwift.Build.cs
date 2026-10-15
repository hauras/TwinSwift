// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TwinSwift : ModuleRules
{
	public TwinSwift(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 헤더에서 타입을 쓰는 모듈 (GAS, 입력, AI)
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"AIModule"
		});

		// cpp에서만 쓰는 모듈
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"NavigationSystem",
			"UMG",
			"Slate",
			"SlateCore",
			"Niagara",
			"MotionWarping"
		});
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "NiagaraEditor", "UnrealEd" });
		}
	}
}
