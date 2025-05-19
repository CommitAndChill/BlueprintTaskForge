#pragma once

#include "CoreMinimal.h"

#include "BtfNameSelect.generated.h"

USTRUCT(BlueprintType)
struct FBtf_NameSelect
{
	GENERATED_BODY()

	FBtf_NameSelect() : Name(NAME_None) {}
	FBtf_NameSelect(FName InName) : Name(InName) {}

	UPROPERTY(EditAnywhere, Category = "NameSelect")
	FName Name;

	FORCEINLINE operator FName() const { return Name; }
	FORCEINLINE friend uint32 GetTypeHash(const FBtf_NameSelect& Node) { return GetTypeHash(Node.Name); }
	friend FArchive& operator<<(FArchive& Ar, FBtf_NameSelect& Node)
	{
		Ar << Node.Name;
		return Ar;
	}

#if WITH_EDITORONLY_DATA
	TSet<FName>* All = nullptr;
	TArray<FBtf_NameSelect>* Exclude = nullptr;

	FORCEINLINE FBtf_NameSelect& operator=(const FBtf_NameSelect& Other)
	{
		Name = Other.Name;
		All = Other.All;
		Exclude = Other.Exclude;
		return *this;
	}
	FORCEINLINE FBtf_NameSelect& operator=(const FName& Other)
	{
		Name = Other;
		return *this;
	}

	FORCEINLINE bool operator==(const FBtf_NameSelect& Other) const { return Name == Other.Name; }
	FORCEINLINE bool operator==(const FName& Other) const { return Name == Other; }

	FORCEINLINE void SetAllExclude(TSet<FName>& InAll, TArray<FBtf_NameSelect>& InExclude)
	{
		All = &InAll;
		Exclude = &InExclude;
	}
#endif
};
