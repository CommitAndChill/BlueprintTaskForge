//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintAITaskTemplate.h"
#include "Engine/EngineBaseTypes.h"
#include "BlueprintTaskTemplateTickable.h"

#include "BlueprintAITaskTemplate_Tickable.generated.h"


USTRUCT(BlueprintType)
struct FAITask_TickFunction : public FTickFunction //48 bytes
{
	GENERATED_BODY()

	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	virtual FString DiagnosticMessage() override;
	virtual FName DiagnosticContext(bool bDetailed) override;

	class UBlueprintAITaskTemplate_Tickable* Owner;
};
template<>
struct TStructOpsTypeTraits<FAITask_TickFunction> : public TStructOpsTypeTraitsBase2<FAITask_TickFunction>
{
	enum
	{
		WithCopy = false
	};
};


/** Blueprint AI Task Template Tickable*/
UCLASS(Abstract, Blueprintable, BlueprintType)
class BLUEPRINTNODETEMPLATE_API UBlueprintAITaskTemplate_Tickable : public UBlueprintAITaskTemplate
{
	GENERATED_BODY()
	friend FAITask_TickFunction;

public:
	UBlueprintAITaskTemplate_Tickable(const FObjectInitializer& ObjectInitializer);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tick")
	struct FAITask_TickFunction TickFunc;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tick")
	ETickPrerequisite_BP TickPrerequisite = ETickPrerequisite_BP::TickAfterOwner;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tick")
	bool bIsTickPausable = true;

	virtual void Tick(float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "BlueprintAITask", meta = (DisplayName = "Tick"))
	void ReceiveTick(float DeltaSeconds);

	UFUNCTION() void RegTick();
	UFUNCTION() void UnRegTick();
	UFUNCTION() void PauseTick();
	UFUNCTION() void UnPauseTick();

	UFUNCTION(BlueprintCallable, Category = "BlueprintAITask")
	void SetTickEnable(bool bEnable) { TickFunc.SetTickFunctionEnable(bEnable); }
};
