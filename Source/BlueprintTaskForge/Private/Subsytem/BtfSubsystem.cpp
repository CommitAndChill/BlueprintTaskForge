#include "Subsystem/BtfSubsystem.h"

#include "BtfTaskForge.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UBtf_WorldSubsystem::
    Deinitialize()
    -> void
{
#if WITH_EDITOR
    if (IsValid(GEngine))
    {
        if (const auto& BlueprintTaskEngineSubsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
            IsValid(BlueprintTaskEngineSubsystem))
        {
            BlueprintTaskEngineSubsystem->Request_Clear();
        }
    }
#endif

    Super::Deinitialize();
}

auto
    UBtf_WorldSubsystem::
    Request_TrackTask(
        UBtf_TaskForge* Task)
    -> void
{
    QUICK_SCOPE_CYCLE_COUNTER(Request_TrackTask)

    if (!IsValid(Task))
    { return; }

    _BlueprintTasks.Add(Task);

    if(FBtf_OutersBlueprintTasksArrayWrapper* TasksWrapper = ObjectsAndTheirTasks.Find(Task->GetOuter()))
    {
        TasksWrapper->Tasks.AddUnique(Task);
    }
    else
    {
        FBtf_OutersBlueprintTasksArrayWrapper TasksArrayWrapper;
        TasksArrayWrapper.Tasks.Add(Task);
        ObjectsAndTheirTasks.Add(Task->GetOuter(), TasksArrayWrapper);
    }
}

auto
    UBtf_WorldSubsystem::
    Request_UntrackTask(
        UBtf_TaskForge* Task)
    -> void
{
    QUICK_SCOPE_CYCLE_COUNTER(Request_UntrackTask)

    if (!IsValid(Task))
    { return; }

    _BlueprintTasks.Remove(Task);

    if(FBtf_OutersBlueprintTasksArrayWrapper* TasksWrapper = ObjectsAndTheirTasks.Find(Task->GetOuter()))
    {
        TasksWrapper->Tasks.RemoveSingle(Task);
        if(TasksWrapper->Tasks.IsEmpty())
        {
            ObjectsAndTheirTasks.Remove(Task->GetOuter());
        }
    }
}

auto
    UBtf_EngineSubsystem::
    Request_Add(
        FGuid InTaskNodeGuid,
        UBtf_TaskForge* InTaskInstance)
    -> void
{
#if WITH_EDITOR
    _TaskNodeGuidToTaskInstance.Add(InTaskNodeGuid, TWeakObjectPtr<UBtf_TaskForge>(InTaskInstance));
    _TaskInstanceTaskNodeGuid.Add(InTaskInstance, InTaskNodeGuid);
#endif
}

auto
    UBtf_EngineSubsystem::
    Request_Remove(
        UBtf_TaskForge* InTaskInstance)
    -> void
{
#if WITH_EDITOR
    if (const auto& FoundTaskNodeGuid = _TaskInstanceTaskNodeGuid.Find(InTaskInstance);
        FoundTaskNodeGuid != nullptr)
    {
        _TaskNodeGuidToTaskInstance.Remove(*FoundTaskNodeGuid);
        _TaskInstanceTaskNodeGuid.Remove(InTaskInstance);
    }
#endif
}

auto
    UBtf_EngineSubsystem::
    Request_Remove(
        FGuid InTaskNodeGuid)
    -> void
{
#if WITH_EDITOR
    if (const auto& FoundTaskInstance = _TaskNodeGuidToTaskInstance.Find(InTaskNodeGuid);
        FoundTaskInstance != nullptr)
    {
        const auto& TaskInstance = *FoundTaskInstance;
        _TaskInstanceTaskNodeGuid.Remove(TaskInstance);
        _TaskNodeGuidToTaskInstance.Remove(InTaskNodeGuid);
    }
#endif
}

auto
    UBtf_EngineSubsystem::
    Request_Clear()
    -> void
{
#if WITH_EDITOR
    _TaskNodeGuidToTaskInstance.Empty();
    _TaskInstanceTaskNodeGuid.Empty();
#endif
}

auto
    UBtf_EngineSubsystem::
    FindTaskInstanceWithGuid(
        FGuid InTaskNodeGuid)
    -> UBtf_TaskForge*
{
#if WITH_EDITOR
    if (const auto& FoundTaskInstance = _TaskNodeGuidToTaskInstance.Find(InTaskNodeGuid);
        FoundTaskInstance != nullptr)
    {
        return FoundTaskInstance->Get();
    }

    return {};
#else
    return {};
#endif
}

// --------------------------------------------------------------------------------------------------------------------
