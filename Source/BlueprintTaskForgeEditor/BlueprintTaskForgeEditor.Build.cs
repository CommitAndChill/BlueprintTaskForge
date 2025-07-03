// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

using UnrealBuildTool;

public class BlueprintTaskForgeEditor : ModuleRules
{
	public BlueprintTaskForgeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{

			});


		PrivateIncludePaths.AddRange(
			new string[]
			{

			});


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"GraphEditor",
				"BlueprintGraph",
				"BlueprintTaskForge",
			});


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
                "UnrealEd",
                "KismetCompiler",
                "GameplayTasksEditor",
				"BlueprintGraph",
				"AssetRegistry",
				"BlueprintTaskForge",

				"Slate",
				"EditorStyle",
				"GraphEditor",
                "SlateCore",
                "ToolMenus",
                "Kismet",
                "KismetWidgets",
                "PropertyEditor",
				"UMG",
				"GameplayTasks",
				"AIModule",

				"DeveloperSettings"

			});


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{

            });
    }
}
