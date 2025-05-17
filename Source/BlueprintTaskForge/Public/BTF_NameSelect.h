#pragma once

#include "CoreMinimal.h"

#include "BTF_NameSelect.generated.h"

USTRUCT(BlueprintType)
struct FBTF_NameSelect
{
	GENERATED_BODY()

	FBTF_NameSelect() : Name(NAME_None) {}
	FBTF_NameSelect(FName InName) : Name(InName) {}

	UPROPERTY(EditAnywhere, Category = "NameSelect")
	FName Name;

	FORCEINLINE operator FName() const { return Name; }
	FORCEINLINE friend uint32 GetTypeHash(const FBTF_NameSelect& Node) { return GetTypeHash(Node.Name); }
	friend FArchive& operator<<(FArchive& Ar, FBTF_NameSelect& Node)
	{
		Ar << Node.Name;
		return Ar;
	}

#if WITH_EDITORONLY_DATA
	TSet<FName>* All = nullptr;
	TArray<FBTF_NameSelect>* Exclude = nullptr;

	FORCEINLINE FBTF_NameSelect& operator=(const FBTF_NameSelect& Other)
	{
		Name = Other.Name;
		All = Other.All;
		Exclude = Other.Exclude;
		return *this;
	}
	FORCEINLINE FBTF_NameSelect& operator=(const FName& Other)
	{
		Name = Other;
		return *this;
	}

	FORCEINLINE bool operator==(const FBTF_NameSelect& Other) const { return Name == Other.Name; }
	FORCEINLINE bool operator==(const FName& Other) const { return Name == Other; }

	FORCEINLINE void SetAllExclude(TSet<FName>& InAll, TArray<FBTF_NameSelect>& InExclude)
	{
		All = &InAll;
		Exclude = &InExclude;
	}
#endif
};
