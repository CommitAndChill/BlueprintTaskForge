// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "BtfEditorSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Blueprint Task Forge Editor Settings"))
class BLUEPRINTTASKFORGEEDITOR_API UBtf_EditorSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
	/* The task palette will automatically retrieve all task nodes,
	 * but your project might contain functions that are related
	 * to tasks that you want to add to the task palette.
	 * This array allows you to add whatever function you
	 * want to that palette. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
    TArray<FName> ExtraTaskPaletteFunctions;

    virtual FName GetSectionName() const override;
    virtual FName GetCategoryName() const override;
};

// --------------------------------------------------------------------------------------------------------------------