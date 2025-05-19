#pragma once

#include "Btf_TaskForge.h"

#include <Subsystems/EngineSubsystem.h>
#include <Subsystems/WorldSubsystem.h>

#include "Btf_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------
USTRUCT()
struct FOutersBlueprintTasksArrayWrapper
{
	GENERATED_BODY()

	TArray<TWeakObjectPtr<class UBtf_TaskForge>> Tasks;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class BLUEPRINTTASKFORGE_API UBtf_WorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

    auto Deinitialize() -> void override;

public:
    auto
    Request_TrackTask(
        UBtf_TaskForge* InTask) -> void;

    auto
    Request_UntrackTask(
        UBtf_TaskForge* InTask) -> void;

private:
    UPROPERTY(Transient)
    TSet<TObjectPtr<UBtf_TaskForge>> _BlueprintTasks;

    UPROPERTY()
    TMap<TWeakObjectPtr<UObject>, FOutersBlueprintTasksArrayWrapper> ObjectsAndTheirTasks;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class BLUEPRINTTASKFORGE_API UBtf_EngineSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    auto
    Request_Add(
        FGuid InTaskNodeGuid,
        UBtf_TaskForge* InTaskInstance) -> void;

    auto
    Request_Remove(
        UBtf_TaskForge* InTaskInstance) -> void;

    auto
    Request_Remove(
        FGuid InTaskNodeGuid) -> void;

    auto
    Request_Clear() -> void;

    auto
    FindTaskInstanceWithGuid(
        FGuid InTaskNodeGuid) -> UBtf_TaskForge*;

private:
#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TMap<FGuid, TWeakObjectPtr<UBtf_TaskForge>> _TaskNodeGuidToTaskInstance;

    UPROPERTY()
    TMap<TWeakObjectPtr<UBtf_TaskForge>, FGuid> _TaskInstanceTaskNodeGuid;
# endif
};

// --------------------------------------------------------------------------------------------------------------------
