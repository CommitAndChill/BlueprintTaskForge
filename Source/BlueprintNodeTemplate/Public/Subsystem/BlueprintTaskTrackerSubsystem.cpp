#include "Subsystem/BlueprintTaskTrackerSubsystem.h"

#include "BlueprintTaskTemplate.h"

void UBlueprintTaskTrackerSubsystem::AddTask(UBlueprintTaskTemplate* Task)
{
	#if !UE_BUILD_SHIPPING
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
	#endif
}

void UBlueprintTaskTrackerSubsystem::RemoveTask(UBlueprintTaskTemplate* Task)
{
	#if !UE_BUILD_SHIPPING
	if(FOutersBlueprintTasksArrayWrapper* TasksWrapper = ObjectsAndTheirTasks.Find(Task->GetOuter()))
	{
		TasksWrapper->Tasks.RemoveSingle(Task);
		if(TasksWrapper->Tasks.IsEmpty())
		{
			ObjectsAndTheirTasks.Remove(Task->GetOuter());
		}
	}
	#endif
}
