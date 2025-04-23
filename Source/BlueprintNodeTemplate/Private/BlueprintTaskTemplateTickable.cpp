//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "BlueprintTaskTemplateTickable.h"

#include "Engine/World.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"


void FBlueprintTaskTickFunction::ExecuteTick(const float DeltaTime, ELevelTick, ENamedThreads::Type, const FGraphEventRef&)
{
	check(Owner);
	Owner->Tick(DeltaTime);
}

FString FBlueprintTaskTickFunction::DiagnosticMessage()
{
	return TEXT("UBlueprintTaskTemplateTickable::Tick");
}

FName FBlueprintTaskTickFunction::DiagnosticContext(bool bDetailed)
{
	return FName(TEXT("BlueprintTaskTemplateTickable"));
}

UBlueprintTaskTemplateTickable::UBlueprintTaskTemplateTickable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TickFunc.Owner = this;
	TickFunc.bCanEverTick = true;
	TickFunc.bStartWithTickEnabled = false;
}

void UBlueprintTaskTemplateTickable::Tick(const float DeltaTime)
{
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		ReceiveTick(DeltaTime);
	}
}

void UBlueprintTaskTemplateTickable::Activate_Internal()
{
	UObject* Owner = GetOuter();
	if (IsValid(Owner))
	{
		if (TickFunc.bCanEverTick)
		{
			FTickFunction* OwnerTickFunctionPtr = nullptr;
			if (const auto Actor = Cast<AActor>(Owner))
			{
				Actor->PrimaryActorTick.AddPrerequisite(this, TickFunc);
			}
			else if (const auto Component = Cast<UActorComponent>(Owner))
			{
				OwnerTickFunctionPtr = &Component->PrimaryComponentTick;
			}

			if (OwnerTickFunctionPtr)
			{
				switch (TickPrerequisite)
				{
					case ETickPrerequisite_BP::IndependentTick:
					{
						break;
					}
					case ETickPrerequisite_BP::TickBeforeOwner:
					{
						(*OwnerTickFunctionPtr).AddPrerequisite(this, TickFunc);
						break;
					}
					case ETickPrerequisite_BP::TickAfterOwner:
					{
						TickFunc.AddPrerequisite(Owner, *OwnerTickFunctionPtr);
						break;
					}
					default: break;
				}
			}
			TickFunc.RegisterTickFunction(GetWorld()->PersistentLevel);
		}
		Activate_BP();
	}
}

void UBlueprintTaskTemplateTickable::BeginDestroy()
{
	TickFunc.UnRegisterTickFunction();
	Super::BeginDestroy();
}
