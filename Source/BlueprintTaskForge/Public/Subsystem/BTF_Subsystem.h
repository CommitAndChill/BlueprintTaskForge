#pragma once

#include "BTF_TaskForge.h"

#include <Subsystems/EngineSubsystem.h>
#include <Subsystems/WorldSubsystem.h>

#include "BTF_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------
USTRUCT()
struct FOutersBlueprintTasksArrayWrapper
{
	GENERATED_BODY()

	TArray<TWeakObjectPtr<class UBTF_TaskForge>> Tasks;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class BLUEPRINTTASKFORGE_API UBTF_WorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

    auto Deinitialize() -> void override;

public:
    auto
    Request_TrackTask(
        UBTF_TaskForge* InTask) -> void;

    auto
    Request_UntrackTask(
        UBTF_TaskForge* InTask) -> void;

private:
    UPROPERTY(Transient)
    TSet<TObjectPtr<UBTF_TaskForge>> _BlueprintTasks;

    UPROPERTY()
    TMap<TWeakObjectPtr<UObject>, FOutersBlueprintTasksArrayWrapper> ObjectsAndTheirTasks;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class BLUEPRINTTASKFORGE_API UBTF_EngineSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    auto
    Request_Add(
        FGuid InTaskNodeGuid,
        UBTF_TaskForge* InTaskInstance) -> void;

    auto
    Request_Remove(
        UBTF_TaskForge* InTaskInstance) -> void;

    auto
    Request_Remove(
        FGuid InTaskNodeGuid) -> void;

    auto
    Request_Clear() -> void;

    auto
    FindTaskInstanceWithGuid(
        FGuid InTaskNodeGuid) -> UBTF_TaskForge*;

private:
#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TMap<FGuid, TWeakObjectPtr<UBTF_TaskForge>> _TaskNodeGuidToTaskInstance;

    UPROPERTY()
    TMap<TWeakObjectPtr<UBTF_TaskForge>, FGuid> _TaskInstanceTaskNodeGuid;
# endif
};

// --------------------------------------------------------------------------------------------------------------------
