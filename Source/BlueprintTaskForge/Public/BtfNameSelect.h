#pragma once

#include "CoreMinimal.h"
#include "BtfNameSelect.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct BLUEPRINTTASKFORGE_API FBtf_NameSelect
{
    GENERATED_BODY()

public:
    FBtf_NameSelect() = default;
    explicit FBtf_NameSelect(FName InName);

    operator FName() const;
    friend uint32 GetTypeHash(const FBtf_NameSelect& Node);
    friend FArchive& operator<<(FArchive& Ar, FBtf_NameSelect& Node);

#if WITH_EDITORONLY_DATA
    TSet<FName>* All = nullptr;
    TArray<FBtf_NameSelect>* Exclude = nullptr;

    FBtf_NameSelect& operator=(const FBtf_NameSelect& Other);
    FBtf_NameSelect& operator=(const FName& Other);
    bool operator==(const FBtf_NameSelect& Other) const;
    bool operator==(const FName& Other) const;

    void SetAllExclude(TSet<FName>& InAll, TArray<FBtf_NameSelect>& InExclude);
#endif

    UPROPERTY(EditAnywhere, Category = "NameSelect")
    FName Name = NAME_None;
};

// --------------------------------------------------------------------------------------------------------------------