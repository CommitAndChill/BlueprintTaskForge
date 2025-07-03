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
#include "BtfRuntimeSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Config = Game, DefaultConfig, DisplayName = "Blueprint Task Forge Runtime Settings")
class BLUEPRINTTASKFORGE_API UBtf_RuntimeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /* Inside a tasks class defaults, should "Deactivate" always be
     * enforced in the "Exec Function" array? */
    UPROPERTY(Category = "Node Settings", EditAnywhere, Config)
    bool EnforceDeactivateExecFunction = true;

    virtual FName GetSectionName() const override;
    virtual FName GetCategoryName() const override;
};

// --------------------------------------------------------------------------------------------------------------------