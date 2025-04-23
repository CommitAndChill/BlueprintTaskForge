//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

using UnrealBuildTool;

public class BlueprintNodeTemplate : ModuleRules
{
    public BlueprintNodeTemplate(ReadOnlyTargetRules Target) : base(Target)
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
                "BlueprintNodeTemplate/Private",
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

            });


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {

            });
    }
}

/*
"WhitelistPlatforms":
[
    "Win64",
    "Win32",
    "Mac",
    "Linux",
    "IOS",
    "Android",
    "PS4",
    "XboxOne",
    "HTML5",
    "SteamVR",
    "GearVR",
    "HoloLens",
    "TVOS",
    "Switch",
    "Lumin"
]

"BlacklistPlatforms" :
[
    "IOS",
    "Android",
    "PS4",
    "XboxOne",
    "HTML5",
    "HoloLens",
    "TVOS",
    "Switch",
    "Lumin"
]
*/