// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

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