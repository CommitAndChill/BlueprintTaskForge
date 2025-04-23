//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/AITask.h"
#include "NameSelect.h"
#include "BlueprintAITaskTemplate.generated.h"


class UGameplayTasksComponent;
class UGameplayTask;
class AActor;
class UWorld;
class AAIController;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAITask_Delegate);

/** Blueprint AI Task Template */
UCLASS(Abstract, Blueprintable, BlueprintType, hideFunctions = "EndTask")
class BLUEPRINTNODETEMPLATE_API UBlueprintAITaskTemplate : public UAITask
{
	GENERATED_BODY()

public:
	UBlueprintAITaskTemplate(const FObjectInitializer& ObjectInitializer);

	//AITask_Blueprintable Factory Function
	UFUNCTION(
		BlueprintCallable,
		Category = "AI|BlueprintAITask",
		meta = (
			//WorldContext = "Controller",
			//DefaultToSelf = "Controller",
			DisplayName = "ExtendConstructAiTask",
			BlueprintInternalUseOnly = "TRUE",
			DeterminesOutputType = "Class",
			Keywords = "AI Task Extend Spawn Create Construct"))
	static UBlueprintAITaskTemplate* ExtendConstructAiTask(AAIController* Controller, TSubclassOf<UBlueprintAITaskTemplate> Class);

	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintCallable, Category = "BlueprintAITask", meta = (DisplayName = "StartTask", ExposeAutoCall = "true"))
	void StartTask();
	UFUNCTION(BlueprintCallable, Category = "BlueprintAITask", meta = (DisplayName = "EndTask"))
	void EndTask_BP();
	UFUNCTION(BlueprintCallable, Category = "BlueprintAITask", meta = (DisplayName = "PauseTask"))
	void PauseTask();
	UFUNCTION(BlueprintCallable, Category = "BlueprintAITask", meta = (DisplayName = "ResumeTask"))
	void ResumeTask();

	UFUNCTION(BlueprintCallable, Category = "BlueprintAITask", meta = (DisplayName = "Get AI Controller"))
	AAIController* GetAIController_BP() const;
	UFUNCTION(BlueprintCallable, Category = "BlueprintAITask", meta = (DisplayName = "Get Pawn"))
	APawn* GetPawn() const;

protected:
	virtual void Pause() override { Super::Pause(); }
	virtual void Resume() override { Super::Resume(); }
	virtual void Activate() override { Super::Activate(); }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BlueprintAITask")
	bool IsPausable = true;

	UPROPERTY(EditAnywhere, BlueprintAssignable, Category = "BlueprintAITask")
	FAITask_Delegate OnStartTaskDelegate;
	UPROPERTY(EditAnywhere, BlueprintAssignable, Category = "BlueprintAITask")
	FAITask_Delegate OnPauseTaskDelegate;
	UPROPERTY(EditAnywhere, BlueprintAssignable, Category = "BlueprintAITask")
	FAITask_Delegate OnResumeTaskDelegate;
	UPROPERTY(EditAnywhere, BlueprintAssignable, Category = "BlueprintAITask")
	FAITask_Delegate OnEndTaskDelegate;

	UFUNCTION(BlueprintImplementableEvent, Category = "BlueprintAITask", meta = (DisplayName = "OnStartTask"))
	void On_StartTask_BP();
	UFUNCTION(BlueprintImplementableEvent, Category = "BlueprintAITask", meta = (DisplayName = "OnPauseTask"))
	void On_PauseTask_BP();
	UFUNCTION(BlueprintImplementableEvent, Category = "BlueprintAITask", meta = (DisplayName = "OnResumeTask"))
	void On_ResumeTask_BP();
	UFUNCTION(BlueprintImplementableEvent, Category = "BlueprintAITask", meta = (DisplayName = "OnEndTask"))
	void On_EndTask_BP();

private:
	UPROPERTY(Transient)
	UWorld* WorldPrivate = nullptr;

protected:
	// UE_DEPRECATED(4.25, "AIController nothing to do and does not work with GameplayTasks, and no UGameplayTasksComponent in AIController class by default" )
	// IGameplayTaskOwnerInterface BEGIN
	virtual UGameplayTasksComponent* GetGameplayTasksComponent(const UGameplayTask& Task) const override;
	virtual AActor* GetGameplayTaskOwner(const UGameplayTask* Task) const override;
	virtual AActor* GetGameplayTaskAvatar(const UGameplayTask* Task) const override;
	virtual uint8 GetGameplayTaskDefaultPriority() const override;

	virtual void OnGameplayTaskInitialized(UGameplayTask& Task) override;
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override;
	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;
	// IGameplayTaskOwnerInterface END

	// / ** Should this task run on simulated clients? This should only be used in rare cases, such as movement tasks. Simulated Tasks do not broadcast their end delegates.  * /
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	//	bool IsSimulatedTask = false;
	// / ** Am I actually running this as a simulated task. (This will be true on clients that simulating. This will be false on the server and the owning client) * /
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	//	bool IsSimulating = false;
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	//	bool IsTickingTask = true;

public:
	/*
	UE_DEPRECATED(4.25, "AIController nothing to do and does not work with GameplayTasks, and no UGameplayTasksComponent in AIController class by default")
		UFUNCTION(BlueprintCallable, Category = "AI|BlueprintAITask", meta = (DisplayName = "GetGameplayTasksComponent"))
		UGameplayTasksComponent* GetGameplayTasksComponent_BP() const { return TasksComponent.Get(); }

	/ ** Proper way to get the owning actor of task owner. This can be the owner itself since the owner is given as a interface * /
	UFUNCTION(BlueprintCallable, Category = "AI|BlueprintAITask", meta = (DisplayName = "GetOwnerActor"))
		AActor* GetOwnerActor_BP() const { return GetOwnerActor(); }

	/ ** Proper way to get the avatar actor associated with the task owner (usually a pawn, tower, etc) * /
	UFUNCTION(BlueprintCallable, Category = "AI|BlueprintAITask", meta = (DisplayName = "GetAvatarActor"))
		AActor* GetAvatarActor_BP() const { return GetAvatarActor(); }
	*/
	// UE_DEPRECATED(4.25, "AIController nothing to do and does not work with GameplayTasks, and no UGameplayTasksComponent in AIController class by default" )
	// UGameplayTask BEGIN
	virtual void TickTask(float DeltaTime) override;
	virtual void InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent) override;
	virtual void ExternalConfirm(bool bEndTask) override;
	virtual void ExternalCancel() override;
	virtual FString GetDebugString() const override;
	virtual bool IsSupportedForNetworking() const override { return bSimulatedTask; }
	// UGameplayTask END

#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	// force refresh all for old/new in-editor params/functions etc
	void RefreshCollected();

protected:
	static void CollectSpawnParam(const UClass* InClass, TSet<FName>& Out);
	static void CollectFunctions(const UClass* InClass, TSet<FName>& Out);
	static void CollectDelegates(const UClass* InClass, TSet<FName>& Out);
	static void CleanInvalidParams(TArray<FNameSelect>& Arr, const TSet<FName>& ArrRef);

#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(VisibleDefaultsOnly, Category = "ExposeOptions")
	TSet<FName> AllDelegates;
	UPROPERTY(VisibleDefaultsOnly, Category = "ExposeOptions")
	TSet<FName> AllFunctions;
	UPROPERTY(VisibleDefaultsOnly, Category = "ExposeOptions")
	TSet<FName> AllFunctionsExec;
	UPROPERTY(VisibleDefaultsOnly, Category = "ExposeOptions")
	TSet<FName> AllParam;

	UPROPERTY(EditDefaultsOnly, Category = "ExposeOptions")
	TArray<FNameSelect> SpawnParam;
	UPROPERTY(EditDefaultsOnly, Category = "ExposeOptions")
	TArray<FNameSelect> AutoCallFunction;
	UPROPERTY(EditDefaultsOnly, Category = "ExposeOptions")
	TArray<FNameSelect> ExecFunction;
	UPROPERTY(EditDefaultsOnly, Category = "ExposeOptions")
	TArray<FNameSelect> InDelegate;
	UPROPERTY(EditDefaultsOnly, Category = "ExposeOptions")
	TArray<FNameSelect> OutDelegate;
#endif

	virtual void Serialize(FArchive& Ar) override;
};
