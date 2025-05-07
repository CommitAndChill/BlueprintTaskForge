using UnrealBuildTool;

public class BlueprintTaskForge : ModuleRules
{
    public BlueprintTaskForge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[]
            {
                ModuleDirectory + "/Public",
            });


        PrivateIncludePaths.AddRange(
            new string[]
            {
                "BlueprintTaskForge/Private",
            });


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
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