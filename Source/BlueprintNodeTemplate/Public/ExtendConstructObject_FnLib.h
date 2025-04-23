//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NameSelect.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"

#include "ExtendConstructObject_FnLib.generated.h"

class UObject;
class UUserWidget;
//class UWidget;

#if WITH_EDITORONLY_DATA
struct FSpawnParam
{
	TArray<FNameSelect> SpawnParam;
	TArray<FNameSelect> AutoCallFunction;
	TArray<FNameSelect> ExecFunction;
	TArray<FNameSelect> InDelegate;
	TArray<FNameSelect> OutDelegate;
};
#endif


/**
 * Extend Construct Object Function Library
 */
UCLASS()
class BLUEPRINTNODETEMPLATE_API UExtendConstructObject_FnLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	//Construct Object
	UFUNCTION(
		BlueprintCallable,
		Category = "ConstructTemplate",
		meta =
			(DefaultToSelf = "Outer",
			 DisplayName = "ExtendConstructObject",
			 BlueprintInternalUseOnly = "TRUE",
			 DeterminesOutputType = "Class",
			 Keywords = "Extend Spawn Create Construct Object"))
	static UObject* ExtendConstructObject(UObject* Outer, TSubclassOf<UObject> Class);

	//Construct Actor
	UFUNCTION(
		BlueprintCallable,
		Category = "ConstructTemplate",
		meta =
			(WorldContext = "WorldContextObject",
			 DefaultToSelf = "WorldContextObject",
			 DisplayName = "ExtendSpawnActor",
			 BlueprintInternalUseOnly = "TRUE",
			 DeterminesOutputType = "Class",
			 Keywords = "Extend Spawn Create Construct Actor"))
	static AActor* ExtendSpawnActor(
		UObject* Owner,
		TSubclassOf<class AActor> Class,
		FTransform SpawnTransform,
		ESpawnActorCollisionHandlingMethod CollisionHandling);


	//Construct UserWidget
	UFUNCTION(
		BlueprintCallable,
		Category = "ConstructTemplate",
		meta =
			(WorldContext = "WorldContextObject",
			 DefaultToSelf = "WorldContextObject",
			 DisplayName = "ExtendConstructWidget",
			 BlueprintInternalUseOnly = "TRUE",
			 DeterminesOutputType = "Class",
			 Keywords = "Extend Spawn Create Construct Widget"))
	static UUserWidget* ExtendConstructWidget(UObject* WorldContextObject, TSubclassOf<class UUserWidget> Class, class APlayerController* OwningPlayer);

	/*
	 //todo ExtendConstruct UWidget
	//Widget
	UFUNCTION(BlueprintCallable, Category = "BlueprintTaskTemplate",
		meta = (
			WorldContext = "InWidget",
			DefaultToSelf = "InWidget",
			DisplayName = "ExtendConstructBaseWidget",
			//BlueprintInternalUseOnly = "TRUE",
			DeterminesOutputType = "Class",
			Keywords = "Extend Create  Construct Base Widget Component"
		))
		static UWidget* ExtendConstructBaseWidget(UUserWidget* InWidget, class UPanelWidget* PanelWidget, TSubclassOf<class UWidget> Class);
	*/

	//todo ExtendConstructActorComponent


	static bool GetNumericSuffix(const FString& InStr, int32& Suffix);
	static bool LessSuffix(const FName& A, const FString& AStr, const FName& B, const FString& BStr);
#if WITH_EDITOR
	static FSpawnParam CollectSpawnParam(
		const UClass* InClass,
		const TSet<FName>& AllDelegates,
		const TSet<FName>& AllFunctions,
		const TSet<FName>& AllFunctionsExec,
		const TSet<FName>& AllParam);
#endif
};
