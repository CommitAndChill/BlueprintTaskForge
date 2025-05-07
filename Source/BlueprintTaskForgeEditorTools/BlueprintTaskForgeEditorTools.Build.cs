using UnrealBuildTool;

public class BlueprintTaskForgeEditorTools : ModuleRules
{
    public BlueprintTaskForgeEditorTools(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "EditorWidgets", 
                "BlueprintTaskForgeEditor",
                "BlueprintTaskForge"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "GraphEditor",
                "UnrealEd",
                "Kismet",
                "BlueprintGraph",
                "AIModule"
            }
        );
    }
}