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

#include "BtfTaskForge.h"
#include "BftMacros.h"

#include <Subsystems/EngineSubsystem.h>
#include <Subsystems/WorldSubsystem.h>

#include "BtfSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FBtf_OutersBlueprintTasksArrayWrapper
{
    GENERATED_BODY()

    TArray<TWeakObjectPtr<class UBtf_TaskForge>> Tasks;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class BLUEPRINTTASKFORGE_API UBtf_WorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    void TrackTask(UBtf_TaskForge* InTask);
    void UntrackTask(UBtf_TaskForge* InTask);

    TMap<TWeakObjectPtr<UObject>, FBtf_OutersBlueprintTasksArrayWrapper> GetTaskTree();

private:
    UPROPERTY(Transient)
    TSet<TObjectPtr<UBtf_TaskForge>> BlueprintTasks;

    UPROPERTY()
    TMap<TWeakObjectPtr<UObject>, FBtf_OutersBlueprintTasksArrayWrapper> ObjectsAndTheirTasks;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class BLUEPRINTTASKFORGE_API UBtf_EngineSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    void Add(FGuid InTaskNodeGuid, UBtf_TaskForge* InTaskInstance);
    void Remove(UBtf_TaskForge* InTaskInstance);
    void Remove(FGuid InTaskNodeGuid);
    void Clear();

    UBtf_TaskForge* FindTaskInstanceWithGuid(FGuid InTaskNodeGuid);

private:
#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TMap<FGuid, TWeakObjectPtr<UBtf_TaskForge>> TaskNodeGuidToTaskInstance;

    UPROPERTY()
    TMap<TWeakObjectPtr<UBtf_TaskForge>, FGuid> TaskInstanceTaskNodeGuid;
# endif
};

// --------------------------------------------------------------------------------------------------------------------