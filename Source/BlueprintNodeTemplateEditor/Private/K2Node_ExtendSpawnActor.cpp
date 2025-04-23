//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.


#include "K2Node_ExtendSpawnActor.h"

#include "ExtendConstructObject_FnLib.h"

#define LOCTEXT_NAMESPACE "K2Node_SpawnActor"

static const FName SpawnTransformPinName(TEXT("SpawnTransform"));
static const FName NoCollisionFailPinName(TEXT("SpawnEvenIfColliding"));

struct FK2Node_ExtendSpawnActorHelper
{
	static const FName SpawnTransformPinName;
	static const FName CollisionHandlingOverridePinName;
};

const FName FK2Node_ExtendSpawnActorHelper::SpawnTransformPinName(TEXT("SpawnTransform"));
const FName FK2Node_ExtendSpawnActorHelper::CollisionHandlingOverridePinName(TEXT("CollisionHandling"));

UK2Node_ExtendSpawnActor::UK2Node_ExtendSpawnActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UExtendConstructObject_FnLib, ExtendSpawnActor);
	ProxyFactoryClass = UExtendConstructObject_FnLib::StaticClass();
	OutPutObjectPinName = FName(TEXT("Actor"));
	WorldContextPinName = FName(TEXT("Owner"));
}


FText UK2Node_ExtendSpawnActor::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (ProxyClass)
	{
		const FString Str = ProxyClass->GetName();
		TArray<FString> ParseNames;
		Str.ParseIntoArray(ParseNames, TEXT("_C"));
		return FText::FromString(FString::Printf(TEXT("ExtendSpawnActor_%s"), *ParseNames[0]));
	}
	return FText(LOCTEXT("UK2Node_ExtendSpawnActor", "ExtendSpawnActor"));
}

UEdGraphPin* UK2Node_ExtendSpawnActor::GetSpawnTransformPin() const
{
	return FindPin(FK2Node_ExtendSpawnActorHelper::SpawnTransformPinName);
}

UEdGraphPin* UK2Node_ExtendSpawnActor::GetCollisionHandlingOverridePin() const
{
	return FindPin(FK2Node_ExtendSpawnActorHelper::CollisionHandlingOverridePinName);
}


void UK2Node_ExtendSpawnActor::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
}


#undef LOCTEXT_NAMESPACE