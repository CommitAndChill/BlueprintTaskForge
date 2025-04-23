//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.


#include "ExtendConstructObject_FnLib.h"

#include "UObject/Object.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

//#include "Components/Widget.h"
//#include "Blueprint/WidgetTree.h"

UObject* UExtendConstructObject_FnLib::ExtendConstructObject(UObject* Outer, const TSubclassOf<UObject> Class)
{
	if (IsValid(Outer) && Class && !Class->HasAnyClassFlags(CLASS_Abstract))
	{
		const FName TaskObjName = MakeUniqueObjectName(Outer, Class, Class->GetFName());
		const auto Task = NewObject<UObject>(Outer, Class, TaskObjName, RF_NoFlags);
		return Task;
	}
	return nullptr;
}

UUserWidget* UExtendConstructObject_FnLib::ExtendConstructWidget(
	UObject* WorldContextObject,
	const TSubclassOf<UUserWidget> Class,
	class APlayerController* OwningPlayer)
{
	if (IsValid(OwningPlayer) && IsValid(WorldContextObject) && Class && !Class->HasAnyClassFlags(CLASS_Abstract))
	{
		if (OwningPlayer)
		{
			return CreateWidget(OwningPlayer, Class);
		}
		if (APlayerController* ImpliedOwningPlayer = Cast<APlayerController>(WorldContextObject))
		{
			return CreateWidget(ImpliedOwningPlayer, Class);
		}
		if (UUserWidget* OwningWidget = Cast<UUserWidget>(WorldContextObject))
		{
			return CreateWidget(OwningWidget, Class);
		}
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			return CreateWidget(World, Class);
		}
	}
	return nullptr;
}

bool UExtendConstructObject_FnLib::GetNumericSuffix(const FString& InStr, int32& Suffix)
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
bool UExtendConstructObject_FnLib::LessSuffix(const FName& A, const FString& AStr, const FName& B, const FString& BStr)
{
	int32 a, b;
	if (GetNumericSuffix(AStr, a) && GetNumericSuffix(BStr, b))
	{
		return a < b;
	}
	return A.FastLess(B);
}


AActor* UExtendConstructObject_FnLib::ExtendSpawnActor(
	UObject* Owner,
	const TSubclassOf<class AActor> Class,
	const FTransform SpawnTransform,
	const ESpawnActorCollisionHandlingMethod CollisionHandling)
{
	if (IsValid(Owner) && Class && !Class->HasAnyClassFlags(CLASS_Abstract))
	{
		AActor* const Actor = UGameplayStatics::BeginDeferredActorSpawnFromClass(Owner, Class, SpawnTransform, CollisionHandling);
		if (Actor)
		{
			Actor->FinishSpawning(SpawnTransform);
		}
		return Actor;
	}
	return nullptr;
}


/*
UWidget* UExtendConstructObject_FnLib::ExtendConstructBaseWidget(UUserWidget* InWidget, UPanelWidget* PanelWidget, TSubclassOf<class UWidget> Class)
{
	
	if (InWidget == nullptr || Class == nullptr) return nullptr;
	UWidget* W = InWidget->WidgetTree->ConstructWidget<UWidget>(Class);
	
	if(PanelWidget) PanelWidget->AddChild(W);
	
	return W;
}
*/
#if WITH_EDITOR
FSpawnParam UExtendConstructObject_FnLib::CollectSpawnParam(
	const UClass* InClass,
	const TSet<FName>& AllDelegates,
	const TSet<FName>& AllFunctions,
	const TSet<FName>& AllFunctionsExec,
	const TSet<FName>& AllParam)
{

	static const FString InPrefix = FString::Printf(TEXT("In_"));
	static const FString OutPrefix = FString::Printf(TEXT("Out_"));
	static const FString InitPrefix = FString::Printf(TEXT("Init_"));

	FSpawnParam Out;

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
