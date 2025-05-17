#include "Subsystem/BTF_Subsystem.h"

#include "BTF_TaskForge.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UBTF_WorldSubsystem::
    Deinitialize()
    -> void
{
#if WITH_EDITOR
    if (IsValid(GEngine))
    {
        if (const auto& BlueprintTaskEngineSubsystem = GEngine->GetEngineSubsystem<UBTF_EngineSubsystem>();
            IsValid(BlueprintTaskEngineSubsystem))
        {
            BlueprintTaskEngineSubsystem->Request_Clear();
        }
    }
#endif

    Super::Deinitialize();
}

auto
    UBTF_WorldSubsystem::
    Request_TrackTask(
        UBTF_TaskForge* Task)
    -> void
{
    QUICK_SCOPE_CYCLE_COUNTER(Request_TrackTask)

    if (!IsValid(Task))
    { return; }

    _BlueprintTasks.Add(Task);

    if(FOutersBlueprintTasksArrayWrapper* TasksWrapper = ObjectsAndTheirTasks.Find(Task->GetOuter()))
	{
		TasksWrapper->Tasks.AddUnique(Task);
	}
	else
	{
		FOutersBlueprintTasksArrayWrapper TasksArrayWrapper;
		TasksArrayWrapper.Tasks.Add(Task);
		ObjectsAndTheirTasks.Add(Task->GetOuter(), TasksArrayWrapper);
	}
}

auto
    UBTF_WorldSubsystem::
    Request_UntrackTask(
        UBTF_TaskForge* Task)
    -> void
{
    QUICK_SCOPE_CYCLE_COUNTER(Request_UntrackTask)

    if (!IsValid(Task))
    { return; }

    _BlueprintTasks.Remove(Task);

    if(FOutersBlueprintTasksArrayWrapper* TasksWrapper = ObjectsAndTheirTasks.Find(Task->GetOuter()))
	{
		TasksWrapper->Tasks.RemoveSingle(Task);
		if(TasksWrapper->Tasks.IsEmpty())
		{
			ObjectsAndTheirTasks.Remove(Task->GetOuter());
		}
	}
}

auto
    UBTF_EngineSubsystem::
    Request_Add(
        FGuid InTaskNodeGuid,
        UBTF_TaskForge* InTaskInstance)
    -> void
{
#if WITH_EDITOR
    _TaskNodeGuidToTaskInstance.Add(InTaskNodeGuid, TWeakObjectPtr<UBTF_TaskForge>(InTaskInstance));
    _TaskInstanceTaskNodeGuid.Add(InTaskInstance, InTaskNodeGuid);
#endif
}

auto
    UBTF_EngineSubsystem::
    Request_Remove(
        UBTF_TaskForge* InTaskInstance)
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
    UBTF_EngineSubsystem::
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
    UBTF_EngineSubsystem::
    Request_Clear()
    -> void
{
#if WITH_EDITOR
    _TaskNodeGuidToTaskInstance.Empty();
    _TaskInstanceTaskNodeGuid.Empty();
#endif
}

auto
    UBTF_EngineSubsystem::
    FindTaskInstanceWithGuid(
        FGuid InTaskNodeGuid)
    -> UBTF_TaskForge*
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
