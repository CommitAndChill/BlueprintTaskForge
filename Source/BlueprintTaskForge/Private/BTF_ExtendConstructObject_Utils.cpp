#include "Btf_ExtendConstructObject_Utils.h"

#include "UObject/Object.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

UObject* UBTF_ExtendConstructObject_Utils::ExtendConstructObject(UObject* Outer, const TSubclassOf<UObject> Class)
{
	if (IsValid(Outer) && Class && !Class->HasAnyClassFlags(CLASS_Abstract))
	{
		const FName TaskObjName = MakeUniqueObjectName(Outer, Class, Class->GetFName());
		const auto Task = NewObject<UObject>(Outer, Class, TaskObjName, RF_NoFlags);
		return Task;
	}
	return nullptr;
}

bool UBTF_ExtendConstructObject_Utils::GetNumericSuffix(const FString& InStr, int32& Suffix)
{
	const TCHAR* Str = *InStr + InStr.Len() - 1;
	int32 SufLength = 0;
	while (FChar::IsDigit(*Str))
	{
		++SufLength;
		--Str;
	}
	if (SufLength > 0)
	{
		Suffix = FCString::Atoi(*InStr.Mid(InStr.Len() - SufLength, InStr.Len() - 1));
		return true;
	}
	Suffix = MAX_int32;
	return false;
}
bool UBTF_ExtendConstructObject_Utils::LessSuffix(const FName& A, const FString& AStr, const FName& B, const FString& BStr)
{
	int32 a, b;
	if (GetNumericSuffix(AStr, a) && GetNumericSuffix(BStr, b))
	{
		return a < b;
	}
	return A.FastLess(B);
}

#if WITH_EDITOR
FBTF_SpawnParam UBTF_ExtendConstructObject_Utils::CollectSpawnParam(
	const UClass* InClass,
	const TSet<FName>& AllDelegates,
	const TSet<FName>& AllFunctions,
	const TSet<FName>& AllFunctionsExec,
	const TSet<FName>& AllParam)
{

	static const FString InPrefix = FString::Printf(TEXT("In_"));
	static const FString OutPrefix = FString::Printf(TEXT("Out_"));
	static const FString InitPrefix = FString::Printf(TEXT("Init_"));

	FBTF_SpawnParam Out;

	//params
	for (const FName& It : AllParam)
	{
		const FString ItStr = It.ToString();
		if (const FProperty* Property = InClass->FindPropertyByName(It))
		{
			if (Property->HasAllPropertyFlags(CPF_BlueprintVisible))
			{
				const bool bIsExposedToSpawn = Property->HasMetaData(TEXT("ExposeOnSpawn")) || Property->HasAllPropertyFlags(CPF_ExposeOnSpawn);
				if (bIsExposedToSpawn || ItStr.StartsWith(InPrefix))
				{
					Out.SpawnParam.AddUnique(It);
				}
			}
		}
	}
	//functions
	for (const FName& It : AllFunctions)
	{
		const FString ItStr = It.ToString();
		if (const UFunction* Function = InClass->FindFunctionByName(It))
		{
			const bool bIn = //
				Function->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public) &&
				!Function->HasAnyFunctionFlags(
					FUNC_BlueprintPure | FUNC_Const | FUNC_Static | FUNC_UbergraphFunction | FUNC_Delegate | FUNC_Private | FUNC_Protected | FUNC_EditorOnly) &&
				!Function->GetBoolMetaData(TEXT("BlueprintInternalUseOnly")) && !Function->HasMetaData(TEXT("DeprecatedFunction"));
			if (bIn)
			{
				if (bIn && (Function->GetBoolMetaData(FName("ExposeAutoCall")) || ItStr.StartsWith(InitPrefix)))
				{
					Out.AutoCallFunction.AddUnique(It);
				}
				else if (ItStr.StartsWith(InPrefix))
				{
					Out.ExecFunction.AddUnique(It);
				}
			}
		}
	}

	Algo::Sort(
		Out.AutoCallFunction, //
		[](const FName& A, const FName& B)
		{
			const FString AStr = A.ToString();
			const FString BStr = B.ToString();
			const bool bA_Init = AStr.StartsWith(InitPrefix);
			const bool bB_Init = BStr.StartsWith(InitPrefix);
			return (bA_Init && !bB_Init) || //
				((bA_Init && bB_Init) ? LessSuffix(A, AStr, B, BStr) : A.FastLess(B));
		});

	//Delegates
	for (const FName& It : AllDelegates)
	{
		const FString ItStr = It.ToString();
		if (ItStr.StartsWith(InPrefix))
		{
			if (const FMulticastDelegateProperty* Delegate = CastField<FMulticastDelegateProperty>(InClass->FindPropertyByName(It)))
			{
				Out.InDelegate.AddUnique(It);
			}
		}
		else if (ItStr.StartsWith(OutPrefix))
		{
			if (const FMulticastDelegateProperty* Delegate = CastField<FMulticastDelegateProperty>(InClass->FindPropertyByName(It)))
			{
				Out.OutDelegate.AddUnique(It);
			}
		}
	}


	return Out;
}
#endif
