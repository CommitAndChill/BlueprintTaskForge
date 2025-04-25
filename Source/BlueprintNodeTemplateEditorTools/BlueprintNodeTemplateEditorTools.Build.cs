using UnrealBuildTool;

public class BlueprintNodeTemplateEditorTools : ModuleRules
{
    public BlueprintNodeTemplateEditorTools(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "EditorWidgets", "BlueprintNodeTemplateEditor",
                "BlueprintNodeTemplate"
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