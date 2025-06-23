#include "BtfNameSelect.h"

FBtf_NameSelect::FBtf_NameSelect() : Name(NAME_None)
{
}

FBtf_NameSelect::FBtf_NameSelect(FName InName) : Name(InName)
{
}

FBtf_NameSelect::operator FName() const
{
    return Name;
}

uint32 GetTypeHash(const FBtf_NameSelect& Node)
{
    return GetTypeHash(Node.Name);
}

FArchive& operator<<(FArchive& Ar, FBtf_NameSelect& Node)
{
    Ar << Node.Name;
    return Ar;
}

#if WITH_EDITORONLY_DATA
FBtf_NameSelect& FBtf_NameSelect::operator=(const FBtf_NameSelect& Other)
{
    Name = Other.Name;
    All = Other.All;
    Exclude = Other.Exclude;
    return *this;
}

FBtf_NameSelect& FBtf_NameSelect::operator=(const FName& Other)
{
    Name = Other;
    return *this;
}

bool FBtf_NameSelect::operator==(const FBtf_NameSelect& Other) const
{
    return Name == Other.Name;
}

bool FBtf_NameSelect::operator==(const FName& Other) const
{
    return Name == Other;
}

void FBtf_NameSelect::SetAllExclude(TSet<FName>& InAll, TArray<FBtf_NameSelect>& InExclude)
{
    All = &InAll;
    Exclude = &InExclude;
}
#endif