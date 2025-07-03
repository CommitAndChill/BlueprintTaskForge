// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

#pragma once

#include "CoreMinimal.h"
#include "BtfNameSelect.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"

#include "BtfExtendConstructObject_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

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
    static FBtf_SpawnParam CollectSpawnParam(
        const UClass* InClass,
        const TSet<FName>& AllDelegates,
        const TSet<FName>& AllFunctions,
        const TSet<FName>& AllFunctionsExec,
        const TSet<FName>& AllParam);
#endif
};

// --------------------------------------------------------------------------------------------------------------------