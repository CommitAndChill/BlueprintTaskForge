#pragma once

#include "BtfTaskForge.h"
#include "BftMacros.h"

#include <Subsystems/EngineSubsystem.h>
#include <Subsystems/WorldSubsystem.h>

#include "BtfSubsystem.generated.h"

USTRUCT()
struct FBtf_OutersBlueprintTasksArrayWrapper
{
    GENERATED_BODY()

    UPROPERTY(meta = (AllowPrivateAccess = "true"))
    TArray<TWeakObjectPtr<class UBtf_TaskForge>> Tasks;

public:
    BFT_PROPERTY_GET(Tasks)
};

UCLASS()
class BLUEPRINTTASKFORGE_API UBtf_WorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

    auto Deinitialize() -> void override;

public:
    auto Request_TrackTask(UBtf_TaskForge* InTask) -> void;
    auto Request_UntrackTask(UBtf_TaskForge* InTask) -> void;

    TMap<TWeakObjectPtr<UObject>, FBtf_OutersBlueprintTasksArrayWrapper> GetTaskTree()
    {
        return ObjectsAndTheirTasks;
    }

private:
    UPROPERTY(Transient, meta = (AllowPrivateAccess = "true"))
    TSet<TObjectPtr<UBtf_TaskForge>> BlueprintTasks;

    UPROPERTY(meta = (AllowPrivateAccess = "true"))
    TMap<TWeakObjectPtr<UObject>, FBtf_OutersBlueprintTasksArrayWrapper> ObjectsAndTheirTasks;

public:
    BFT_PROPERTY_GET(BlueprintTasks)
    BFT_PROPERTY_GET(ObjectsAndTheirTasks)
};

UCLASS()
class BLUEPRINTTASKFORGE_API UBtf_EngineSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    auto Request_Add(FGuid InTaskNodeGuid, UBtf_TaskForge* InTaskInstance) -> void;
    auto Request_Remove(UBtf_TaskForge* InTaskInstance) -> void;
    auto Request_Remove(FGuid InTaskNodeGuid) -> void;
    auto Request_Clear() -> void;
    auto FindTaskInstanceWithGuid(FGuid InTaskNodeGuid) -> UBtf_TaskForge*;

private:
#if WITH_EDITORONLY_DATA
    UPROPERTY(meta = (AllowPrivateAccess = "true"))
    TMap<FGuid, TWeakObjectPtr<UBtf_TaskForge>> TaskNodeGuidToTaskInstance;

    UPROPERTY(meta = (AllowPrivateAccess = "true"))
    TMap<TWeakObjectPtr<UBtf_TaskForge>, FGuid> TaskInstanceTaskNodeGuid;

public:
    BFT_PROPERTY_GET(TaskNodeGuidToTaskInstance)
    BFT_PROPERTY_GET(TaskInstanceTaskNodeGuid)
#endif
};