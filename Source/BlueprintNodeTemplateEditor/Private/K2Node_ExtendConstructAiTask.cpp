//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.


#include "K2Node_ExtendConstructAiTask.h"

#include "BlueprintAITaskTemplate.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet2/BlueprintEditorUtils.h"


#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_ExtendConstructAiTask::UK2Node_ExtendConstructAiTask(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UBlueprintAITaskTemplate, ExtendConstructAiTask);
	ProxyFactoryClass = UBlueprintAITaskTemplate::StaticClass();
	OutPutObjectPinName = FName(TEXT("AITask"));
	WorldContextPinName = FName(TEXT("Controller"));
}

FText UK2Node_ExtendConstructAiTask::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (ProxyClass)
	{
		const FString Str = ProxyClass->GetName();
		TArray<FString> ParseNames;
		Str.ParseIntoArray(ParseNames, TEXT("_C"));
		return FText::FromString(ParseNames[0]);
	}
	return FText(LOCTEXT("UK2Node_ExtendConstructAiTask", "ConstructAiTask Function"));
}

#if WITH_EDITORONLY_DATA
void UK2Node_ExtendConstructAiTask::ResetToDefaultExposeOptions_Impl()
{
	if (ProxyClass)
	{
		if (const auto CDO = ProxyClass->GetDefaultObject<UBlueprintAITaskTemplate>())
		{
			AllDelegates = CDO->AllDelegates;
			AllFunctions = CDO->AllFunctions;
			AllFunctionsExec = CDO->AllFunctionsExec;
			AllParam = CDO->AllParam;

			SpawnParam = CDO->SpawnParam;
			AutoCallFunction = CDO->AutoCallFunction;
			ExecFunction = CDO->ExecFunction;
			InDelegate = CDO->InDelegate;
			OutDelegate = CDO->OutDelegate;
		}
	}
	ReconstructNode();
}
#endif

void UK2Node_ExtendConstructAiTask::CollectSpawnParam(UClass* InClass, const bool bFullRefresh)
{
	if (InClass)
	{
		if (const UBlueprintAITaskTemplate* CDO = InClass->GetDefaultObject<UBlueprintAITaskTemplate>())
		{
			AllDelegates = CDO->AllDelegates;
			AllFunctions = CDO->AllFunctions;
			AllFunctionsExec = CDO->AllFunctionsExec;
			AllParam = CDO->AllParam;
			if (bFullRefresh)
			{
				SpawnParam = CDO->SpawnParam;
				AutoCallFunction = CDO->AutoCallFunction;
				ExecFunction = CDO->ExecFunction;
				InDelegate = CDO->InDelegate;
				OutDelegate = CDO->OutDelegate;
			}
		}
	}
}
#undef LOCTEXT_NAMESPACE
