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
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "Misc/Guid.h"
#include "Serialization/CustomVersion.h"

// --------------------------------------------------------------------------------------------------------------------

class FBlueprintTaskForgeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

// --------------------------------------------------------------------------------------------------------------------

struct FBlueprintTaskForgeCustomVersion
{
    enum Type
    {
        BeforeCustomVersionWasAdded = 0,
        ExposeOnSpawnInClass = 1,
        Btf_UE5_1,
        VersionPlusOne,
        LatestVersion = VersionPlusOne - 1
    };

    BLUEPRINTTASKFORGE_API const static FGuid GUID;
};

// --------------------------------------------------------------------------------------------------------------------