#include "BlueprintTaskForge_Module.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FBlueprintTaskForgeModule"

const FGuid FBlueprintTaskForgeCustomVersion::GUID = FGuid(
    GetTypeHash(FString(TEXT("Blueprint"))),
    GetTypeHash(FString(TEXT("Task"))),
    GetTypeHash(FString(TEXT("Forge"))),
    GetTypeHash(FString(TEXT("Plugin"))));

void FBlueprintTaskForgeModule::StartupModule()
{
}

void FBlueprintTaskForgeModule::ShutdownModule()
{
}

FCustomVersionRegistration GRegisterBlueprintTaskForgeVersion(
    FBlueprintTaskForgeCustomVersion::GUID,
    FBlueprintTaskForgeCustomVersion::LatestVersion,
    TEXT("BlueprintTaskForge"));

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintTaskForgeModule, BlueprintTaskForge)

// --------------------------------------------------------------------------------------------------------------------