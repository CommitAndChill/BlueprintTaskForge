#include "Subsystem/BtfSubsystem.h"
#include "BtfTaskForge.h"

// --------------------------------------------------------------------------------------------------------------------

void UBtf_WorldSubsystem::Deinitialize()
{
#if WITH_EDITOR
    if (IsValid(GEngine))
    {
        if (const auto& BlueprintTaskEngineSubsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
            IsValid(BlueprintTaskEngineSubsystem))
        {
            BlueprintTaskEngineSubsystem->Clear();
        }
    }
#endif

    Super::Deinitialize();
}

void UBtf_WorldSubsystem::TrackTask(UBtf_TaskForge* Task)
{
    QUICK_SCOPE_CYCLE_COUNTER(TrackTask)

    if (NOT IsValid(Task))
    { return; }

    BlueprintTasks.Add(Task);

    if (auto* TasksWrapper = ObjectsAndTheirTasks.Find(Task->GetOuter()))
    {
        TasksWrapper->Tasks.AddUnique(Task);
    }
    else
    {
        auto TasksArrayWrapper = FBtf_OutersBlueprintTasksArrayWrapper{};
        TasksArrayWrapper.Tasks.Add(Task);
        ObjectsAndTheirTasks.Add(Task->GetOuter(), TasksArrayWrapper);
    }
}

void UBtf_WorldSubsystem::UntrackTask(UBtf_TaskForge* Task)
{
    QUICK_SCOPE_CYCLE_COUNTER(UntrackTask)

    if (NOT IsValid(Task))
    { return; }

    BlueprintTasks.Remove(Task);

    if (auto* TasksWrapper = ObjectsAndTheirTasks.Find(Task->GetOuter()))
    {
        TasksWrapper->Tasks.RemoveSingle(Task);
        if (TasksWrapper->Tasks.IsEmpty())
        {
            ObjectsAndTheirTasks.Remove(Task->GetOuter());
        }
    }
}

TMap<TWeakObjectPtr<UObject>, FBtf_OutersBlueprintTasksArrayWrapper> UBtf_WorldSubsystem::GetTaskTree()
{
    return ObjectsAndTheirTasks;
}

void UBtf_EngineSubsystem::Add(FGuid InTaskNodeGuid, UBtf_TaskForge* InTaskInstance)
{
#if WITH_EDITOR
    TaskNodeGuidToTaskInstance.Add(InTaskNodeGuid, TWeakObjectPtr<UBtf_TaskForge>(InTaskInstance));
    TaskInstanceTaskNodeGuid.Add(InTaskInstance, InTaskNodeGuid);
#endif
}

void UBtf_EngineSubsystem::Remove(UBtf_TaskForge* InTaskInstance)
{
#if WITH_EDITOR
    if (const auto& FoundTaskNodeGuid = TaskInstanceTaskNodeGuid.Find(InTaskInstance);
        FoundTaskNodeGuid != nullptr)
    {
        TaskNodeGuidToTaskInstance.Remove(*FoundTaskNodeGuid);
        TaskInstanceTaskNodeGuid.Remove(InTaskInstance);
    }
#endif
}

void UBtf_EngineSubsystem::Remove(FGuid InTaskNodeGuid)
{
#if WITH_EDITOR
    if (const auto& FoundTaskInstance = TaskNodeGuidToTaskInstance.Find(InTaskNodeGuid);
        FoundTaskInstance != nullptr)
    {
        const auto& TaskInstance = *FoundTaskInstance;
        TaskInstanceTaskNodeGuid.Remove(TaskInstance);
        TaskNodeGuidToTaskInstance.Remove(InTaskNodeGuid);
    }
#endif
}

void UBtf_EngineSubsystem::Clear()
{
#if WITH_EDITOR
    TaskNodeGuidToTaskInstance.Empty();
    TaskInstanceTaskNodeGuid.Empty();
#endif
}

UBtf_TaskForge* UBtf_EngineSubsystem::FindTaskInstanceWithGuid(FGuid InTaskNodeGuid)
{
#if WITH_EDITOR
    if (const auto& FoundTaskInstance = TaskNodeGuidToTaskInstance.Find(InTaskNodeGuid);
        FoundTaskInstance != nullptr)
    {
        return FoundTaskInstance->Get();
    }
#endif

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------