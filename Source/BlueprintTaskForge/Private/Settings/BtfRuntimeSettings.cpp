// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

#include "Settings/BtfRuntimeSettings.h"

// --------------------------------------------------------------------------------------------------------------------

FName UBtf_RuntimeSettings::GetSectionName() const
{
    return "Blueprint Task Forge Runtime Settings";
}

FName UBtf_RuntimeSettings::GetCategoryName() const
{
    return "Plugins";
}

// --------------------------------------------------------------------------------------------------------------------