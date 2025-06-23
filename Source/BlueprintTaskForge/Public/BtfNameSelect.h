#pragma once

#include "CoreMinimal.h"
#include "BftMacros.h"

#include "BtfNameSelect.generated.h"

USTRUCT(BlueprintType)
struct FBtf_NameSelect
{
    GENERATED_BODY()

    FBtf_NameSelect();
    FBtf_NameSelect(FName InName);

    operator FName() const;
    friend uint32 GetTypeHash(const FBtf_NameSelect& Node);
    friend FArchive& operator<<(FArchive& Ar, FBtf_NameSelect& Node);

private:
    UPROPERTY(EditAnywhere, Category = "NameSelect", meta = (AllowPrivateAccess = "true"))
    FName Name;

#if WITH_EDITORONLY_DATA
    TSet<FName>* All = nullptr;
    TArray<FBtf_NameSelect>* Exclude = nullptr;
#endif

public:
    BFT_PROPERTY_GET(Name)

#if WITH_EDITORONLY_DATA
    FBtf_NameSelect& operator=(const FBtf_NameSelect& Other);
    FBtf_NameSelect& operator=(const FName& Other);
    bool operator==(const FBtf_NameSelect& Other) const;
    bool operator==(const FName& Other) const;
    void SetAllExclude(TSet<FName>& InAll, TArray<FBtf_NameSelect>& InExclude);

    BFT_PROPERTY_GET(All)
    BFT_PROPERTY_GET(Exclude)
#endif
};