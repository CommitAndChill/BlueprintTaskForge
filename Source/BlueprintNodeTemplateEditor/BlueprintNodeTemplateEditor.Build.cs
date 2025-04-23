//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

using UnrealBuildTool;

public class BlueprintNodeTemplateEditor : ModuleRules
{
    public BlueprintNodeTemplateEditor(ReadOnlyTargetRules Target) : base(Target)
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
                "BlueprintNodeTemplate",
//++CK
                "GraphEditor",
//--CK

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
            });


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {

            });
    }
}
