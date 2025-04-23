//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "BlueprintAITaskTemplate.h"
#include "BlueprintNodeTemplate.h"
#include "UObject/UObjectGlobals.h"
#include "GameFramework/Actor.h"
#include "AIController.h"
#include "ExtendConstructObject_FnLib.h"
#include "Engine/World.h"
#include "GameplayTaskOwnerInterface.h"
#include "GameplayTask.h"
#include "GameplayTasksComponent.h"
#include "Tasks/AITask.h"
#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#endif // WITH_EDITOR


UBlueprintAITaskTemplate::UBlueprintAITaskTemplate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsPausable = IsPausable;// IsPausable;
	//bTickingTask	= false;//IsTickingTask;
	
	//bSimulatedTask	= false;//IsSimulatedTask;
	//bIsSimulating	= false;//IsSimulating;
	//bOwnedByTasksComponent	= false;
	//bClaimRequiredResources = false;
	//bOwnerFinished			= false;
	
	TaskState				= EGameplayTaskState::AwaitingActivation;
	ResourceOverlapPolicy	= ETaskResourceOverlapPolicy::StartOnTop;
	Priority				= uint8(EAITaskPriority::AutonomousAI);

	SetFlags(RF_StrongRefOnFrame);

	OnStartTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate::On_StartTask_BP);
	OnEndTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate::On_EndTask_BP);
	if (IsPausable)
	{
		OnPauseTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate::On_PauseTask_BP);
		OnResumeTaskDelegate.AddUniqueDynamic(this, &UBlueprintAITaskTemplate::On_ResumeTask_BP);
	}


	//UGameplayTaskResource
	//AddRequiredResource(UGameplayTaskResource::StaticClass());
	//AddClaimedResource(UGameplayTaskResource::StaticClass());
}

UBlueprintAITaskTemplate* UBlueprintAITaskTemplate::ExtendConstructAiTask(AAIController* Controller, const TSubclassOf<UBlueprintAITaskTemplate> Class)
{
	UBlueprintAITaskTemplate* AITask = nullptr;
	if (IsValid(Controller) && Class && !Class->HasAnyClassFlags(CLASS_Abstract))
	{
		const FName TaskName = Class->GetFName();
		AITask = NewObject<UBlueprintAITaskTemplate>(Controller, Class, TaskName, RF_NoFlags);
		AITask->InstanceName = TaskName;
		AITask->InitAITask(*Controller, *Controller/*->GetGameplayTasksComponent()*/); //, (uint8)EAITaskPriority::AutonomousAI

		if (AITask)
		{

		}
	}
	return AITask;
}



UWorld* UBlueprintAITaskTemplate::GetWorld() const
{
	return WorldPrivate;
}

AAIController* UBlueprintAITaskTemplate::GetAIController_BP() const
{
	return GetAIController();
}

APawn* UBlueprintAITaskTemplate::GetPawn() const
{
	return GetAIController()->GetPawn();
}


void UBlueprintAITaskTemplate::StartTask()
{
	if (GetAIController())
	{
		WorldPrivate = GetAIController()->GetWorld();
		OnStartTaskDelegate.Broadcast();
		Activate();
	}
}

void UBlueprintAITaskTemplate::EndTask_BP()
{
	OnEndTaskDelegate.Broadcast();
	EndTask();
}

void UBlueprintAITaskTemplate::PauseTask()
{
	if (bIsPausable)
	{
		OnPauseTaskDelegate.Broadcast();
		Pause();
	}
}

void UBlueprintAITaskTemplate::ResumeTask()
{
	if (bIsPausable)
	{
		OnResumeTaskDelegate.Broadcast();
		Resume();
	}
}


void UBlueprintAITaskTemplate::InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent)
{
	Super::InitSimulatedTask(InGameplayTasksComponent);
}

void UBlueprintAITaskTemplate::ExternalConfirm(bool bEndTask)
{
	Super::ExternalConfirm(bEndTask);
}

void UBlueprintAITaskTemplate::ExternalCancel()
{
	Super::ExternalCancel();
}

FString UBlueprintAITaskTemplate::GetDebugString() const
{
	return Super::GetDebugString();
}

void UBlueprintAITaskTemplate::TickTask(const float DeltaTime)
{
	Super::TickTask(DeltaTime);
	//todo: no UGameplayTasksComponent in AIController class by default
	//ReceiveTick(DeltaTime);
}

UGameplayTasksComponent* UBlueprintAITaskTemplate::GetGameplayTasksComponent(const UGameplayTask& Task) const
{
	return Super::GetGameplayTasksComponent(Task);
}

AActor* UBlueprintAITaskTemplate::GetGameplayTaskOwner(const UGameplayTask* Task) const
{
	return Super::GetGameplayTaskOwner(Task);
}

AActor* UBlueprintAITaskTemplate::GetGameplayTaskAvatar(const UGameplayTask* Task) const
{
	return Super::GetGameplayTaskAvatar(Task);
}

uint8 UBlueprintAITaskTemplate::GetGameplayTaskDefaultPriority() const
{
	return Super::GetGameplayTaskDefaultPriority();
}

void UBlueprintAITaskTemplate::OnGameplayTaskInitialized(UGameplayTask& Task)
{
	Super::OnGameplayTaskInitialized(Task);
	WorldPrivate = Super::GetWorld();
}

void UBlueprintAITaskTemplate::OnGameplayTaskActivated(UGameplayTask& Task)
{
	Super::OnGameplayTaskActivated(Task);
}

void UBlueprintAITaskTemplate::OnGameplayTaskDeactivated(UGameplayTask& Task)
{
	Super::OnGameplayTaskDeactivated(Task);
}

#if WITH_EDITOR
void UBlueprintAITaskTemplate::CollectSpawnParam(const UClass* InClass, TSet<FName>& Out)
{
	Out.Reset();
	for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		const FProperty* Property = *PropertyIt;
		const bool bIsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
		const bool bIsExposedToSpawn = Property->HasMetaData(TEXT("ExposeOnSpawn")) || Property->HasAllPropertyFlags(CPF_ExposeOnSpawn);
		const bool bIsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

		if (!Property->HasAnyPropertyFlags(CPF_Parm) && !bIsDelegate)
		{
			if (bIsExposedToSpawn && bIsSettableExternally && Property->HasAllPropertyFlags(CPF_BlueprintVisible))
			{
				Out.Add(Property->GetFName());
			}
			else if (
				!Property->HasAnyPropertyFlags(			 //
					CPF_NativeAccessSpecifierProtected | //
					CPF_NativeAccessSpecifierPrivate |	 //
					CPF_Protected |						 //
					CPF_BlueprintReadOnly |				 //
					CPF_EditorOnly |					 //
					CPF_InstancedReference |			 //
					CPF_Deprecated |					 //
					CPF_ExportObject) &&				 //
				Property->HasAllPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
			{
				Out.Add(Property->GetFName());
			}
		}
	}
}
void UBlueprintAITaskTemplate::CollectFunctions(const UClass* InClass, TSet<FName>& Out)
{
	Out.Reset();
	for (TFieldIterator<UField> It(InClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		if (const UFunction* LocFunction = Cast<UFunction>(*It))
		{
			if (LocFunction->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public) && //
				!LocFunction->GetBoolMetaData(FName(TEXT("BlueprintInternalUseOnly"))) && //
				!LocFunction->HasMetaData(FName(TEXT("DeprecatedFunction"))) &&			  //
				!FObjectEditorUtils::IsFunctionHiddenFromClass(LocFunction, InClass) &&	  //!InClass->IsFunctionHidden(*LocFunction->GetName())
				!LocFunction->HasAnyFunctionFlags(										  //
					FUNC_Static |														  //
					FUNC_UbergraphFunction |											  //
					FUNC_Delegate |														  //
					FUNC_Private |														  //
					FUNC_Protected |													  //
					FUNC_EditorOnly |													  //
					FUNC_BlueprintPure |												  //
					FUNC_Const))														  // FUNC_BlueprintPure
			{
				Out.Add(LocFunction->GetFName());
			}
		}
	}
}
void UBlueprintAITaskTemplate::CollectDelegates(const UClass* InClass, TSet<FName>& Out)
{
	Out.Reset();
	for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		if (const FMulticastDelegateProperty* DelegateProperty = CastField<FMulticastDelegateProperty>(*PropertyIt))
		{
			if (DelegateProperty->HasAnyPropertyFlags(FUNC_Private | CPF_Protected | FUNC_EditorOnly) || //
				!DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintAssignable) ||						 //
				DelegateProperty->GetBoolMetaData(FName(TEXT("BlueprintInternalUseOnly"))) ||			 //
				DelegateProperty->HasMetaData(TEXT("DeprecatedFunction")))
			{
				continue;
			}

			if (const UFunction* LocFunction = DelegateProperty->SignatureFunction)
			{
				if (!LocFunction->HasAllFunctionFlags(FUNC_Public) ||				  //
					LocFunction->GetBoolMetaData(TEXT("BlueprintInternalUseOnly")) || //
					LocFunction->HasMetaData(TEXT("DeprecatedFunction")) ||
					LocFunction->HasAnyFunctionFlags( //
						FUNC_Static |				  //
						FUNC_BlueprintPure |		  //
						FUNC_Const |				  //
						FUNC_UbergraphFunction |	  //
						FUNC_Private |				  //
						FUNC_Protected |			  //
						FUNC_EditorOnly))
				{
					continue;
				}
			}
			Out.Add(DelegateProperty->GetFName());
		}
	}
}
void UBlueprintAITaskTemplate::CleanInvalidParams(TArray<FNameSelect>& Arr, const TSet<FName>& ArrRef)
{
	for (int32 i = Arr.Num() - 1; i >= 0; --i)
	{
		if (Arr[i].Name != NAME_None && !ArrRef.Contains(Arr[i]))
		{
			Arr.RemoveAt(i, 1, false);
		}
	}
}



void UBlueprintAITaskTemplate::RefreshCollected()
{
	#if WITH_EDITORONLY_DATA
	{
		const UClass* InClass = GetClass();
		CollectSpawnParam(InClass, AllParam);
		CollectFunctions(InClass, AllFunctions);
		CollectDelegates(InClass, AllDelegates);
		AllFunctionsExec = AllFunctions;
		AutoCallFunction.AddUnique(FName(TEXT("StartTask")));
		CleanInvalidParams(AutoCallFunction, AllFunctions);
		CleanInvalidParams(ExecFunction, AllFunctions);
		CleanInvalidParams(InDelegate, AllDelegates);
		CleanInvalidParams(OutDelegate, AllDelegates);
		CleanInvalidParams(SpawnParam, AllParam);

		for (auto& It : AutoCallFunction)
		{
			It.SetAllExclude(AllFunctions, AutoCallFunction);
			ExecFunction.Remove(It);
		}
		for (auto& It : ExecFunction)
		{
			It.SetAllExclude(AllFunctionsExec, ExecFunction);
			AutoCallFunction.Remove(It);
		}
		for (auto& It : InDelegate)
		{
			It.SetAllExclude(AllDelegates, InDelegate);
			OutDelegate.Remove(It);
		}
		for (auto& It : OutDelegate)
		{
			It.SetAllExclude(AllDelegates, OutDelegate);
			InDelegate.Remove(It);
		}
		for (auto& It : SpawnParam)
		{
			It.SetAllExclude(AllParam, SpawnParam);
		}
		AutoCallFunction.AddUnique(FName(TEXT("StartTask")));//todo
	}
	#endif // WITH_EDITORONLY_DATA
}

void UBlueprintAITaskTemplate::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	RefreshCollected();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

void UBlueprintAITaskTemplate::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FBlueprintNodeTemplateCustomVersion::GUID);
	Super::Serialize(Ar);
#if WITH_EDITOR
	if (Ar.IsLoading() && GetLinkerCustomVersion(FBlueprintNodeTemplateCustomVersion::GUID) < FBlueprintNodeTemplateCustomVersion::ExposeOnSpawnInClass)
	{
		RefreshCollected();
		const FSpawnParam Spawn = UExtendConstructObject_FnLib::CollectSpawnParam(GetClass(), AllDelegates, AllFunctions, AllFunctionsExec, AllParam);
		for (const auto& It : Spawn.AutoCallFunction)
		{
			AutoCallFunction.AddUnique(It);
		}
		for (const auto& It : Spawn.ExecFunction)
		{
			ExecFunction.AddUnique(It);
		}
		for (const auto& It : Spawn.InDelegate)
		{
			InDelegate.AddUnique(It);
		}
		for (const auto& It : Spawn.OutDelegate)
		{
			OutDelegate.AddUnique(It);
		}
		for (const auto& It : Spawn.SpawnParam)
		{
			SpawnParam.AddUnique(It);
		}
	}
#endif
}