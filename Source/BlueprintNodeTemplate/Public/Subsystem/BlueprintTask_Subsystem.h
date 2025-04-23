#pragma once

#include <Subsystems/EngineSubsystem.h>

#include "BlueprintTask_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class BLUEPRINTNODETEMPLATE_API UBlueprintTask_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

    auto Deinitialize() -> void override;

public:
    auto
    Request_TrackTask(
        class UBlueprintTaskTemplate* InTask) -> void;

    auto
    Request_UntrackTask(
        class UBlueprintTaskTemplate* InTask) -> void;

private:
    UPROPERTY(Transient)
    TSet<TObjectPtr<class UBlueprintTaskTemplate>> _BlueprintTasks;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class BLUEPRINTNODETEMPLATE_API UUtils_BlueprintTask_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UBlueprintTask_Subsystem_UE;

public:
    static auto
    Request_TrackTask(
        const UWorld* InWorld,
        class UBlueprintTaskTemplate* Task) -> void;

    static auto
    Request_UntrackTask(
        const UWorld* InWorld,
        class UBlueprintTaskTemplate* Task) -> void;
};

UCLASS()
class BLUEPRINTNODETEMPLATE_API UBlueprintTask_EngineSubsystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    auto
    Request_Add(
        FGuid InTaskNodeGuid,
        UBlueprintTaskTemplate* InTaskInstance) -> void;

    auto
    Request_Remove(
        UBlueprintTaskTemplate* InTaskInstance) -> void;

    auto
    Request_Remove(
        FGuid InTaskNodeGuid) -> void;

    auto
    Request_Clear() -> void;

    auto
    FindTaskInstanceWithGuid(
        FGuid InTaskNodeGuid) -> UBlueprintTaskTemplate*;

private:
#if WITH_EDITORONLY_DATA
    TMap<FGuid, TStrongObjectPtr<UBlueprintTaskTemplate>> _TaskNodeGuidToTaskInstance;
    TMap<UBlueprintTaskTemplate*, FGuid> _TaskInstanceTaskNodeGuid;
# endif
};

// --------------------------------------------------------------------------------------------------------------------
