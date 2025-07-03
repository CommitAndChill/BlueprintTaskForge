// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

#include "BtfExtendConstructObject_Utils.h"

#include "BftMacros.h"

#include "UObject/Object.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// --------------------------------------------------------------------------------------------------------------------

UObject* UBtf_ExtendConstructObject_Utils::ExtendConstructObject(UObject* Outer, const TSubclassOf<UObject> Class)
{
    if (IsValid(Outer) && IsValid(Class) && NOT Class->HasAnyClassFlags(CLASS_Abstract))
    {
        const auto TaskObjName = MakeUniqueObjectName(Outer, Class, Class->GetFName());
        const auto Task = NewObject<UObject>(Outer, Class, TaskObjName, RF_NoFlags);
        return Task;
    }
    return nullptr;
}

bool UBtf_ExtendConstructObject_Utils::GetNumericSuffix(const FString& InStr, int32& Suffix)
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

bool UBtf_ExtendConstructObject_Utils::LessSuffix(const FName& A, const FString& AStr, const FName& B, const FString& BStr)
{
    int32 a, b;
    if (GetNumericSuffix(AStr, a) && GetNumericSuffix(BStr, b))
    {
        return a < b;
    }
    return A.FastLess(B);
}

#if WITH_EDITOR
FBtf_SpawnParam UBtf_ExtendConstructObject_Utils::CollectSpawnParam(
    const UClass* InClass,
    const TSet<FName>& AllDelegates,
    const TSet<FName>& AllFunctions,
    const TSet<FName>& AllFunctionsExec,
    const TSet<FName>& AllParam)
{
    static const auto InPrefix = FString::Printf(TEXT("In_"));
    static const auto OutPrefix = FString::Printf(TEXT("Out_"));
    static const auto InitPrefix = FString::Printf(TEXT("Init_"));

    auto Out = FBtf_SpawnParam{};

    for (const auto& It : AllParam)
    {
        const auto ItStr = It.ToString();
        if (const auto* Property = InClass->FindPropertyByName(It))
        {
            if (Property->HasAllPropertyFlags(CPF_BlueprintVisible))
            {
                const auto IsExposedToSpawn = Property->HasMetaData(TEXT("ExposeOnSpawn")) || Property->HasAllPropertyFlags(CPF_ExposeOnSpawn);
                if (IsExposedToSpawn || ItStr.StartsWith(InPrefix))
                {
                    Out.SpawnParam.AddUnique(FBtf_NameSelect{It});
                }
            }
        }
    }

    for (const auto& It : AllFunctions)
    {
        const auto ItStr = It.ToString();
        if (const auto* Function = InClass->FindFunctionByName(It))
        {
            const auto In = Function->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public) &&
                NOT Function->HasAnyFunctionFlags(
                    FUNC_BlueprintPure | FUNC_Const | FUNC_Static | FUNC_UbergraphFunction | FUNC_Delegate | FUNC_Private | FUNC_Protected | FUNC_EditorOnly) &&
                NOT Function->GetBoolMetaData(TEXT("BlueprintInternalUseOnly")) && NOT Function->HasMetaData(TEXT("DeprecatedFunction"));

            if (In)
            {
                if (In && (Function->GetBoolMetaData(FName("ExposeAutoCall")) || ItStr.StartsWith(InitPrefix)))
                {
                    Out.AutoCallFunction.AddUnique(FBtf_NameSelect{It});
                }
                else if (ItStr.StartsWith(InPrefix))
                {
                    Out.ExecFunction.AddUnique(FBtf_NameSelect{It});
                }
            }
        }
    }

    Algo::Sort(
        Out.AutoCallFunction,
        [](const FName& A, const FName& B)
        {
            const auto AStr = A.ToString();
            const auto BStr = B.ToString();
            const auto A_Init = AStr.StartsWith(InitPrefix);
            const auto B_Init = BStr.StartsWith(InitPrefix);
            return (A_Init && NOT B_Init) ||
                ((A_Init && B_Init) ? LessSuffix(A, AStr, B, BStr) : A.FastLess(B));
        });

    for (const auto& It : AllDelegates)
    {
        const auto ItStr = It.ToString();
        if (ItStr.StartsWith(InPrefix))
        {
            if (const auto* Delegate = CastField<FMulticastDelegateProperty>(InClass->FindPropertyByName(It)))
            {
                Out.InDelegate.AddUnique(FBtf_NameSelect{It});
            }
        }
        else if (ItStr.StartsWith(OutPrefix))
        {
            if (const auto* Delegate = CastField<FMulticastDelegateProperty>(InClass->FindPropertyByName(It)))
            {
                Out.OutDelegate.AddUnique(FBtf_NameSelect{It});
            }
        }
    }

    return Out;
}
#endif

// --------------------------------------------------------------------------------------------------------------------