//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "NameSelect.generated.h"


USTRUCT(BlueprintType)
struct FNameSelect
{
	GENERATED_BODY()

	FNameSelect() : Name(NAME_None) {}
	FNameSelect(FName InName) : Name(InName) {}

	UPROPERTY(EditAnywhere, Category = "NameSelect")
	FName Name;

	FORCEINLINE operator FName() const { return Name; }
	FORCEINLINE friend uint32 GetTypeHash(const FNameSelect& Node) { return GetTypeHash(Node.Name); }
	friend FArchive& operator<<(FArchive& Ar, FNameSelect& Node)
	{
		Ar << Node.Name;
		return Ar;
	}

#if WITH_EDITORONLY_DATA
	TSet<FName>* All = nullptr;
	TArray<FNameSelect>* Exclude = nullptr;

	FORCEINLINE FNameSelect& operator=(const FNameSelect& Other)
	{
		Name = Other.Name;
		All = Other.All;
		Exclude = Other.Exclude;
		return *this;
	}
	FORCEINLINE FNameSelect& operator=(const FName& Other)
	{
		Name = Other;
		return *this;
	}

	FORCEINLINE bool operator==(const FNameSelect& Other) const { return Name == Other.Name; }
	FORCEINLINE bool operator==(const FName& Other) const { return Name == Other; }

	FORCEINLINE void SetAllExclude(TSet<FName>& InAll, TArray<FNameSelect>& InExclude)
	{
		All = &InAll;
		Exclude = &InExclude;
	}
#endif
};
