#include "Subsystem/BtfSubsystem.h"
#include "BtfTaskForge.h"

// --------------------------------------------------------------------------------------------------------------------

void UBtf_WorldSubsystem::Deinitialize()
{
#if WITH_EDITOR
    if (NOT IsValid(GEngine))
    {
        Super::Deinitialize();
        return;
    }

    if (const auto* BlueprintTaskEngineSubsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
        IsValid(BlueprintTaskEngineSubsystem))
    {
        BlueprintTaskEngineSubsystem->Clear();
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

    if (auto* TasksWrapper = ObjectsAndTheirTasks.Find(Task->GetOuter()); 
        TasksWrapper != nullptr)
    {
        TasksWrapper->Tasks.AddUnique(Task);
        return;
    }

    auto TasksArrayWrapper = FBtf_OutersBlueprintTasksArrayWrapper{};
    TasksArrayWrapper.Tasks.Add(Task);
    ObjectsAndTheirTasks.Add(Task->GetOuter(), TasksArrayWrapper);
}

void UBtf_WorldSubsystem::UntrackTask(UBtf_TaskForge* Task)
{
    QUICK_SCOPE_CYCLE_COUNTER(UntrackTask)

    if (NOT IsValid(Task))
    { return; }

    BlueprintTasks.Remove(Task);

    auto* TasksWrapper = ObjectsAndTheirTasks.Find(Task->GetOuter());
    if (NOT TasksWrapper)
    { return; }

    TasksWrapper->Tasks.RemoveSingle(Task);
    if (TasksWrapper->Tasks.IsEmpty())
    {
        ObjectsAndTheirTasks.Remove(Task->GetOuter());
    }
}

// --------------------------------------------------------------------------------------------------------------------

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
    const auto* FoundTaskNodeGuid = TaskInstanceTaskNodeGuid.Find(InTaskInstance);
    if (NOT FoundTaskNodeGuid)
    { return; }

    TaskNodeGuidToTaskInstance.Remove(*FoundTaskNodeGuid);
    TaskInstanceTaskNodeGuid.Remove(InTaskInstance);
#endif
}

void UBtf_EngineSubsystem::Remove(FGuid InTaskNodeGuid)
{
#if WITH_EDITOR
    const auto* FoundTaskInstance = TaskNodeGuidToTaskInstance.Find(InTaskNodeGuid);
    if (NOT FoundTaskInstance)
    { return; }

    const auto& TaskInstance = *FoundTaskInstance;
    TaskInstanceTaskNodeGuid.Remove(TaskInstance);
    TaskNodeGuidToTaskInstance.Remove(InTaskNodeGuid);
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
    const auto* FoundTaskInstance = TaskNodeGuidToTaskInstance.Find(InTaskNodeGuid);
    if (NOT FoundTaskInstance)
    { return nullptr; }

    return FoundTaskInstance->Get();
#else
    return nullptr;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
