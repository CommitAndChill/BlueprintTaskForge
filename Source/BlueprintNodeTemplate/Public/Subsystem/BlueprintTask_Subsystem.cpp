#include "BlueprintTask_Subsystem.h"

#include "BlueprintTaskTemplate.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UBlueprintTask_Subsystem_UE::
    Deinitialize()
    -> void
{
#if WITH_EDITOR
    if (IsValid(GEngine))
    {
        if (const auto& BlueprintTaskEngineSubsystem = GEngine->GetEngineSubsystem<UBlueprintTask_EngineSubsystem_UE>();
            IsValid(BlueprintTaskEngineSubsystem))
        {
            BlueprintTaskEngineSubsystem->Request_Clear();
        }
    }
#endif

    Super::Deinitialize();
}

auto
    UBlueprintTask_Subsystem_UE::
    Request_TrackTask(
        UBlueprintTaskTemplate* Task)
    -> void
{
    if (!IsValid(Task))
    { return; }

    _BlueprintTasks.Add(Task);
}

auto
    UBlueprintTask_Subsystem_UE::
    Request_UntrackTask(
        UBlueprintTaskTemplate* Task)
    -> void
{
    QUICK_SCOPE_CYCLE_COUNTER(Request_UntrackTask)

    if (!IsValid(Task))
    { return; }

    _BlueprintTasks.Remove(Task);
}

auto
    UUtils_BlueprintTask_Subsystem_UE::
    Request_TrackTask(
        const UWorld* InWorld,
        UBlueprintTaskTemplate* Task)
        -> void
{
    if (!IsValid(InWorld))
    { return; }

    const auto Subsystem = InWorld->GetSubsystem<SubsystemType>();

    if (!IsValid(Subsystem))
    { return; }

    Subsystem->Request_TrackTask(Task);
}

auto
    UUtils_BlueprintTask_Subsystem_UE::
    Request_UntrackTask(
        const UWorld* InWorld,
        UBlueprintTaskTemplate* Task)
        -> void
{
    if (!IsValid(InWorld))
    { return; }

    const auto Subsystem = InWorld->GetSubsystem<SubsystemType>();

    if (!IsValid(Subsystem))
    { return; }

    Subsystem->Request_UntrackTask(Task);
}

auto
    UBlueprintTask_EngineSubsystem_UE::
    Request_Add(
        FGuid InTaskNodeGuid,
        UBlueprintTaskTemplate* InTaskInstance)
    -> void
{
#if WITH_EDITOR
    _TaskNodeGuidToTaskInstance.Add(InTaskNodeGuid, TStrongObjectPtr(InTaskInstance));
    _TaskInstanceTaskNodeGuid.Add(InTaskInstance, InTaskNodeGuid);
#endif
}

auto
    UBlueprintTask_EngineSubsystem_UE::
    Request_Remove(
        UBlueprintTaskTemplate* InTaskInstance)
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
    UBlueprintTask_EngineSubsystem_UE::
    Request_Remove(
        FGuid InTaskNodeGuid)
    -> void
{
#if WITH_EDITOR
    if (const auto& FoundTaskInstance = _TaskNodeGuidToTaskInstance.Find(InTaskNodeGuid);
        FoundTaskInstance != nullptr)
    {
        _TaskInstanceTaskNodeGuid.Remove(FoundTaskInstance->Get());
        _TaskNodeGuidToTaskInstance.Remove(InTaskNodeGuid);
    }
#endif
}

auto
    UBlueprintTask_EngineSubsystem_UE::
    Request_Clear()
    -> void
{
#if WITH_EDITOR
    _TaskNodeGuidToTaskInstance.Empty();
    _TaskInstanceTaskNodeGuid.Empty();
#endif
}

auto
    UBlueprintTask_EngineSubsystem_UE::
    FindTaskInstanceWithGuid(
        FGuid InTaskNodeGuid)
    -> UBlueprintTaskTemplate*
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
