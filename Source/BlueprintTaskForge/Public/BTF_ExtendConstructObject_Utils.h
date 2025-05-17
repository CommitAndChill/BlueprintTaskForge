#pragma once

#include "CoreMinimal.h"
#include "BTF_NameSelect.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"

#include "BTF_ExtendConstructObject_Utils.generated.h"

class UObject;
class UUserWidget;

#if WITH_EDITORONLY_DATA
struct FBTF_SpawnParam
{
	TArray<FBTF_NameSelect> SpawnParam;
	TArray<FBTF_NameSelect> AutoCallFunction;
	TArray<FBTF_NameSelect> ExecFunction;
	TArray<FBTF_NameSelect> InDelegate;
	TArray<FBTF_NameSelect> OutDelegate;
};
#endif

UCLASS()
class BLUEPRINTTASKFORGE_API UBTF_ExtendConstructObject_Utils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static UObject* ExtendConstructObject(UObject* Outer, TSubclassOf<UObject> Class);

	static bool GetNumericSuffix(const FString& InStr, int32& Suffix);
	static bool LessSuffix(const FName& A, const FString& AStr, const FName& B, const FString& BStr);

#if WITH_EDITOR
	static FBTF_SpawnParam
    CollectSpawnParam(
		const UClass* InClass,
		const TSet<FName>& AllDelegates,
		const TSet<FName>& AllFunctions,
		const TSet<FName>& AllFunctionsExec,
		const TSet<FName>& AllParam);
#endif
};
