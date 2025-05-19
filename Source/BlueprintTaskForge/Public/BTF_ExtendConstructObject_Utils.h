#pragma once

#include "CoreMinimal.h"
#include "Btf_NameSelect.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"

#include "Btf_ExtendConstructObject_Utils.generated.h"

class UObject;
class UUserWidget;

#if WITH_EDITORONLY_DATA
struct FBtf_SpawnParam
{
	TArray<FBtf_NameSelect> SpawnParam;
	TArray<FBtf_NameSelect> AutoCallFunction;
	TArray<FBtf_NameSelect> ExecFunction;
	TArray<FBtf_NameSelect> InDelegate;
	TArray<FBtf_NameSelect> OutDelegate;
};
#endif

UCLASS()
class BLUEPRINTTASKFORGE_API UBtf_ExtendConstructObject_Utils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static UObject* ExtendConstructObject(UObject* Outer, TSubclassOf<UObject> Class);

	static bool GetNumericSuffix(const FString& InStr, int32& Suffix);
	static bool LessSuffix(const FName& A, const FString& AStr, const FName& B, const FString& BStr);

#if WITH_EDITOR
	static FBtf_SpawnParam
    CollectSpawnParam(
		const UClass* InClass,
		const TSet<FName>& AllDelegates,
		const TSet<FName>& AllFunctions,
		const TSet<FName>& AllFunctionsExec,
		const TSet<FName>& AllParam);
#endif
};
