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
