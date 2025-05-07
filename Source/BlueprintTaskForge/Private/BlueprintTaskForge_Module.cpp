#include "BlueprintTaskForge.h"

#define LOCTEXT_NAMESPACE "FBlueprintTaskForgeModule"

const FGuid FBlueprintTaskForgeCustomVersion::GUID = FGuid( //
	GetTypeHash(FString(TEXT("Blueprint"))),				   //
	GetTypeHash(FString(TEXT("Node"))),						   //
	GetTypeHash(FString(TEXT("Task"))),						   //
	GetTypeHash(FString(TEXT("Template"))));				   //

void FBlueprintTaskForgeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FBlueprintTaskForgeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

// Register the custom version with core
FCustomVersionRegistration GRegisterBlueprintTaskForgeVersion(
	FBlueprintTaskForgeCustomVersion::GUID,
	FBlueprintTaskForgeCustomVersion::LatestVersion,
	TEXT("BlueprintTaskForge"));

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintTaskForgeModule, BlueprintTaskForge)