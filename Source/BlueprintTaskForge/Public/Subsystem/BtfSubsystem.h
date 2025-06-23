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

public:
    BFT_PROPERTY_GET(Tasks)

private:
    UPROPERTY(meta = (AllowPrivateAccess = "true"))
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

    TMap<TWeakObjectPtr<UObject>, FBtf_OutersBlueprintTasksArrayWrapper> GetTaskTree()
    {
        return ObjectsAndTheirTasks;
    }

    BFT_PROPERTY_GET(BlueprintTasks)
    BFT_PROPERTY_GET(ObjectsAndTheirTasks)

private:
    UPROPERTY(Transient, meta = (AllowPrivateAccess = "true"))
    TSet<TObjectPtr<UBtf_TaskForge>> BlueprintTasks;

    UPROPERTY(meta = (AllowPrivateAccess = "true"))
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

#if WITH_EDITORONLY_DATA
public:
    BFT_PROPERTY_GET(TaskNodeGuidToTaskInstance)
    BFT_PROPERTY_GET(TaskInstanceTaskNodeGuid)

private:
    UPROPERTY(meta = (AllowPrivateAccess = "true"))
    TMap<FGuid, TWeakObjectPtr<UBtf_TaskForge>> TaskNodeGuidToTaskInstance;

    UPROPERTY(meta = (AllowPrivateAccess = "true"))
    TMap<TWeakObjectPtr<UBtf_TaskForge>, FGuid> TaskInstanceTaskNodeGuid;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
