//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "BlueprintNodeTemplate.h"

#define LOCTEXT_NAMESPACE "FBlueprintNodeTemplateModule"

const FGuid FBlueprintNodeTemplateCustomVersion::GUID = FGuid( //
	GetTypeHash(FString(TEXT("Blueprint"))),				   //
	GetTypeHash(FString(TEXT("Node"))),						   //
	GetTypeHash(FString(TEXT("Task"))),						   //
	GetTypeHash(FString(TEXT("Template"))));				   //

void FBlueprintNodeTemplateModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FBlueprintNodeTemplateModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

// Register the custom version with core
FCustomVersionRegistration GRegisterBlueprintNodeTemplateVersion(
	FBlueprintNodeTemplateCustomVersion::GUID,
	FBlueprintNodeTemplateCustomVersion::LatestVersion,
	TEXT("BlueprintNodeTemplate"));

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintNodeTemplateModule, BlueprintNodeTemplate)