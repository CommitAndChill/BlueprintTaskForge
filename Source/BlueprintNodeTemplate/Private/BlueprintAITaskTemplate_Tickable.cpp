//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "BlueprintAITaskTemplate_Tickable.h"
#include "Engine/World.h"
#include "AIController.h"


void FAITask_TickFunction::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	check(Owner);
	Owner->Tick(DeltaTime);
}

FString FAITask_TickFunction::DiagnosticMessage()
{
	return TEXT("AITask_BlueprintableTickable::Tick");
}

FName FAITask_TickFunction::DiagnosticContext(bool bDetailed)
{
	return FName(TEXT("AITask_BlueprintableTickable"));
}

UBlueprintAITaskTemplate_Tickable::UBlueprintAITaskTemplate_Tickable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TickFunc.Owner = this;
	TickFunc.bCanEverTick = true;
	TickFunc.bStartWithTickEnabled = false;

	OnStartTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate_Tickable::RegTick);
	OnStartTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate_Tickable::On_StartTask_BP);
	OnEndTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate_Tickable::UnRegTick);
	OnEndTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate_Tickable::On_EndTask_BP);
	if (IsPausable)
	{
		OnPauseTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate_Tickable::PauseTick);
		OnPauseTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate_Tickable::On_PauseTask_BP);
		OnResumeTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate_Tickable::UnPauseTick);
		OnResumeTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate_Tickable::On_ResumeTask_BP);
	}
}

void UBlueprintAITaskTemplate_Tickable::Tick(const float DeltaTime)
{
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		ReceiveTick(DeltaTime);
	}
}

void UBlueprintAITaskTemplate_Tickable::RegTick()
{
	if (TickFunc.bCanEverTick && GetAIController())
	{
		if (AAIController* AIController = GetAIController())
		{
			switch (TickPrerequisite)
			{
				case ETickPrerequisite_BP::IndependentTick:
				{
					break;
				}
				case ETickPrerequisite_BP::TickBeforeOwner:
				{
					AIController->PrimaryActorTick.AddPrerequisite(this, TickFunc);
					break;
				}
				case ETickPrerequisite_BP::TickAfterOwner:
				{
					TickFunc.AddPrerequisite(AIController, AIController->PrimaryActorTick);
					break;
				}
				default: break;
			}
			TickFunc.RegisterTickFunction(GetWorld()->PersistentLevel);
		}
	}
}

void UBlueprintAITaskTemplate_Tickable::UnRegTick()
{
	TickFunc.UnRegisterTickFunction();
}

void UBlueprintAITaskTemplate_Tickable::PauseTick()
{
	if (bIsTickPausable)
	{
		SetTickEnable(false);
	}
}

void UBlueprintAITaskTemplate_Tickable::UnPauseTick()
{
	if (bIsTickPausable)
	{
		SetTickEnable(true);
	}
}
