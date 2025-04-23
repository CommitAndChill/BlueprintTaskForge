//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintTaskTemplate.h"
#include "Engine/EngineBaseTypes.h"

#include "BlueprintTaskTemplateTickable.generated.h"


USTRUCT(BlueprintType)
struct FBlueprintTaskTickFunction : public FTickFunction //48 bytes
{
	GENERATED_BODY()

	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	virtual FString DiagnosticMessage() override;
	virtual FName DiagnosticContext(bool bDetailed) override;

	class UBlueprintTaskTemplateTickable* Owner;
};
template<>
struct TStructOpsTypeTraits<FBlueprintTaskTickFunction> : public TStructOpsTypeTraitsBase2<FBlueprintTaskTickFunction>
{
	enum
	{
		WithCopy = false
	};
};

UENUM(BlueprintType, meta = (DisplayName = "TickPrerequisite"))
enum class ETickPrerequisite_BP : uint8
{
	IndependentTick,
	TickBeforeOwner,
	TickAfterOwner
};

/**
 * 
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class BLUEPRINTNODETEMPLATE_API UBlueprintTaskTemplateTickable : public UBlueprintTaskTemplate
{
	GENERATED_BODY()
	friend FBlueprintTaskTickFunction;

public:
	UBlueprintTaskTemplateTickable(const FObjectInitializer& ObjectInitializer);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tick")
	struct FBlueprintTaskTickFunction TickFunc;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tick")
	ETickPrerequisite_BP TickPrerequisite = ETickPrerequisite_BP::TickAfterOwner;

	virtual void Tick(const float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tick", meta = (DisplayName = "Tick"))
	void ReceiveTick(float DeltaSeconds);
	UFUNCTION(BlueprintCallable, Category = "Tick")
	void SetTickEnable(bool bEnable) { TickFunc.SetTickFunctionEnable(bEnable); }

	virtual void Activate_Internal() override;
	virtual void BeginDestroy() override;
};
