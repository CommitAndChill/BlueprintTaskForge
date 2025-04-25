#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "BlueprintTaskTrackerSubsystem.generated.h"

class UBlueprintTaskTemplate;

USTRUCT()
struct FOutersBlueprintTasksArrayWrapper
{
	GENERATED_BODY()

	TArray<TWeakObjectPtr<UBlueprintTaskTemplate>> Tasks;
};

/**
 * Helper subsystem for tracking all objects and what
 * tasks belong to them.
 * Ideally, should not be used for runtime usage. Only editor tools.
 */
UCLASS()
class BLUEPRINTNODETEMPLATE_API UBlueprintTaskTrackerSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:

	void AddTask(UBlueprintTaskTemplate* Task);
	void RemoveTask(UBlueprintTaskTemplate* Task);
	
	TMap<TWeakObjectPtr<UObject>, FOutersBlueprintTasksArrayWrapper> ObjectsAndTheirTasks;
};
