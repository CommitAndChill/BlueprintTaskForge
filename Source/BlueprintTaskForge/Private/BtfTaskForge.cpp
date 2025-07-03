// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

#include "BtfTaskForge.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "BlueprintTaskForge_Module.h"
#include "BtfExtendConstructObject_Utils.h"
#include "Subsystem/BtfSubsystem.h"
#include "Settings/BtfRuntimeSettings.h"

#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#include "Subsystem/BtfSubsystem.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

UBtf_TaskForge::UBtf_TaskForge(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UBtf_TaskForge* UBtf_TaskForge::BlueprintTaskForge(UObject* Outer, const TSubclassOf<UBtf_TaskForge> Class, FString NodeGuidStr)
{
    if (NOT IsValid(Outer) || NOT IsValid(Class) || Class->HasAnyClassFlags(CLASS_Abstract))
    { return nullptr; }

    if (auto* TaskTemplate = GetTaskByNodeGUID(Outer, NodeGuidStr))
    {
        const auto TaskObjName = MakeUniqueObjectName(Outer, Class, Class->GetFName(), EUniqueObjectNameOptions::GloballyUnique);
        const auto Task = NewObject<UBtf_TaskForge>(Outer, Class, TaskObjName, RF_NoFlags, TaskTemplate);

        if (NOT IsValid(Task))
        { return Task; }

        if (const auto& BlueprintTaskEngineSystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
            IsValid(BlueprintTaskEngineSystem))
        {
            BlueprintTaskEngineSystem->Add(FGuid(NodeGuidStr), Task);
        }

        return Task;
    }

    return nullptr;
}

UBtf_TaskForge* UBtf_TaskForge::GetTaskByNodeGUID(UObject* Outer, FString NodeGUID)
{
    for (UClass* TemplateOwnerClass = (Outer != nullptr) ? Outer->GetClass() : Outer->GetClass()
        ; TemplateOwnerClass
        ; TemplateOwnerClass = TemplateOwnerClass->GetSuperClass())
    {
        if (auto* BPGC = Cast<UBlueprintGeneratedClass>(TemplateOwnerClass))
        {
            if (auto* Blueprint = Cast<UBlueprint>(BPGC->ClassGeneratedBy))
            {
                for (TObjectPtr<UBlueprintExtension> Extension : Blueprint->GetExtensions())
                {
                    if (Extension && Extension->IsA<UBtf_TaskForge>() && Extension->GetFName().ToString().Contains(NodeGUID))
                    {
                        return Cast<UBtf_TaskForge>(Extension);
                    }
                }
            }
        }
    }

    return nullptr;
}

void UBtf_TaskForge::Activate()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Activate)

    if (const auto World = GetWorld();
        IsValid(World))
    {
        World->GetSubsystem<UBtf_WorldSubsystem>()->TrackTask(this);
    }

    Activate_Internal();
}

void UBtf_TaskForge::Deactivate()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Deactivate)

    if (NOT IsActive)
    { return; }

    for (const auto& Task : TasksToDeactivateOnDeactivate)
    {
        if (Task.IsValid())
        {
            Task->Deactivate();
        }
    }

    if (const auto World = GetWorld();
        IsValid(World))
    {
        World->GetSubsystem<UBtf_WorldSubsystem>()->UntrackTask(this);
    }

    Deactivate_Internal();
}

void UBtf_TaskForge::OnDestroy()
{
    IsBeingDestroyed = true;
    MarkAsGarbage();
}

void UBtf_TaskForge::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);
#if WITH_EDITOR
    Ar.UsingCustomVersion(FBlueprintTaskForgeCustomVersion::GUID);
    if (Ar.IsLoading() && GetLinkerCustomVersion(FBlueprintTaskForgeCustomVersion::GUID) < FBlueprintTaskForgeCustomVersion::ExposeOnSpawnInClass)
    {
        RefreshCollected();
        const auto Spawn = UBtf_ExtendConstructObject_Utils::CollectSpawnParam(GetClass(), AllDelegates, AllFunctions, AllFunctionsExec, AllParam);
        for (const auto& It : Spawn.AutoCallFunction)
        {
            AutoCallFunction.AddUnique(It);
        }
        for (const auto& It : Spawn.ExecFunction)
        {
            ExecFunction.AddUnique(It);
        }
        for (const auto& It : Spawn.InDelegate)
        {
            InDelegate.AddUnique(It);
        }
        for (const auto& It : Spawn.OutDelegate)
        {
            OutDelegate.AddUnique(It);
        }
        for (const auto& It : Spawn.SpawnParam)
        {
            SpawnParam.AddUnique(It);
        }
    }
#endif
}

void UBtf_TaskForge::SetupAutomaticCleanup()
{
    if (NOT IsValid(GetOuter()))
    { return; }

    if (GetOuter()->IsA<AActor>())
    {
        if (auto* Actor = Cast<AActor>(GetOuter()))
        {
            Actor->OnDestroyed.AddDynamic(this, &UBtf_TaskForge::OnActorOuterDestroyed);
            return;
        }
    }

    if (GetOuter()->IsA<UBtf_TaskForge>())
    {
        if (auto* TaskTemplate = Cast<UBtf_TaskForge>(GetOuter()))
        {
            TaskTemplate->TrackTaskForAutomaticDeactivation(this);
        }
    }
}

void UBtf_TaskForge::Deactivate_Internal()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Deactivate_Internal)
    if (IsBeingDestroyed)
    { return; }

    if (IsValid(GetOuter()))
    {
        Deactivate_BP();
    }

    IsActive = false;

    if (IsValid(GEngine))
    {
        if (const auto& BlueprintTaskEngineSubsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
            IsValid(BlueprintTaskEngineSubsystem))
        {
            BlueprintTaskEngineSubsystem->Remove(this);
        }
    }

    OnDestroy();
}

bool UBtf_TaskForge::Get_StatusBackgroundColor_Implementation(FLinearColor& OutColor) const
{
    OutColor = FLinearColor();
    return false;
}

FString UBtf_TaskForge::Get_NodeDescription_Implementation() const
{
    return FString();
}

TArray<FString> UBtf_TaskForge::ValidateNodeDuringCompilation_Implementation()
{
    return TArray<FString>();
}

void UBtf_TaskForge::DeactivateAllTasksRelatedToObject(UObject* Object)
{
    auto SubObjects = TArray<UObject*>{};
    GetObjectsWithOuter(Object, SubObjects);

    for (auto& CurrentObject : SubObjects)
    {
        if (auto* TaskTemplate = Cast<UBtf_TaskForge>(CurrentObject))
        {
            TaskTemplate->Deactivate();
        }
    }
}

void UBtf_TaskForge::TriggerCustomOutputPin(FName OutputPin, TInstancedStruct<FCustomOutputPinData> Data)
{
    OnCustomPinTriggered.Broadcast(OutputPin, Data);
}

TArray<FCustomOutputPin> UBtf_TaskForge::Get_CustomOutputPins_Implementation() const
{
    return TArray<FCustomOutputPin>();
}

TArray<FName> UBtf_TaskForge::Get_CustomOutputPinNames() const
{
    auto Result = TArray<FName>{};
    for (auto& CurrentPin : Get_CustomOutputPins())
    {
        Result.Add(FName(CurrentPin.PinName));
    }
    return Result;
}

bool UBtf_TaskForge::Get_NodeTitleColor_Implementation(FLinearColor& Color)
{
    Color = FLinearColor();
    return false;
}

bool UBtf_TaskForge::IsExtension() const
{
    for (UObject* TemplateOwnerClass = (GetOuter() != nullptr) ? GetOuter() : nullptr
        ; TemplateOwnerClass
        ; TemplateOwnerClass = TemplateOwnerClass->GetOuter())
    {
        if (auto* BPGC = Cast<UBlueprintGeneratedClass>(TemplateOwnerClass))
        {
            if (auto* Blueprint = Cast<UBlueprint>(BPGC->ClassGeneratedBy))
            {
                return Blueprint->GetExtensions().Contains(this);
            }
        }

        if (auto* Blueprint = Cast<UBlueprint>(TemplateOwnerClass))
        {
            return Blueprint->GetExtensions().Contains(this);
        }
    }

    return false;
}

void UBtf_TaskForge::Activate_Internal()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Activate_Internal)
    if (IsBeingDestroyed)
    { return; }

    if (IsValid(GetOuter()))
    {
        SetupAutomaticCleanup();
        IsActive = true;
        Activate_BP();
    }
}

void UBtf_TaskForge::TrackTaskForAutomaticDeactivation(UBtf_TaskForge* Task)
{
    if (IsValid(Task) && NOT TasksToDeactivateOnDeactivate.Contains(Task))
    {
        TasksToDeactivateOnDeactivate.Add(Task);
    }
}

void UBtf_TaskForge::UntrackTaskForAutomaticDeactivation(UBtf_TaskForge* Task)
{
    if (IsValid(Task) && TasksToDeactivateOnDeactivate.Contains(Task))
    {
        TasksToDeactivateOnDeactivate.RemoveSingle(Task);
    }
}

UWorld* UBtf_TaskForge::GetWorld() const
{
    return IsTemplate() ? nullptr : GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

void UBtf_TaskForge::OnActorOuterDestroyed(AActor* Actor)
{
    Deactivate();
}

FString UBtf_TaskForge::Get_StatusString_Implementation() const
{
    return FString();
}

bool UBtf_TaskForge::Get_IsActive() const
{
    return IsActive;
}

#if WITH_EDITOR
void UBtf_TaskForge::CollectSpawnParam(const UClass* InClass, TSet<FName>& Out)
{
    Out.Reset();
    for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
    {
        const auto* Property = *PropertyIt;
        const auto IsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
        const auto IsExposedToSpawn = Property->HasMetaData(TEXT("ExposeOnSpawn")) || Property->HasAllPropertyFlags(CPF_ExposeOnSpawn);
        const auto IsSettableExternally = NOT Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

        if (Property->GetMetaData(TEXT("BlueprintPrivate")) == TEXT("true"))
        { continue; }

        if (NOT Property->HasAnyPropertyFlags(CPF_Parm) && NOT IsDelegate)
        {
            if (IsExposedToSpawn && IsSettableExternally && Property->HasAllPropertyFlags(CPF_BlueprintVisible))
            {
                Out.Add(Property->GetFName());
            }
            else if (
                NOT Property->HasAnyPropertyFlags(
                    CPF_NativeAccessSpecifierProtected |
                    CPF_NativeAccessSpecifierPrivate |
                    CPF_Protected |
                    CPF_BlueprintReadOnly |
                    CPF_EditorOnly |
                    CPF_InstancedReference |
                    CPF_Deprecated |
                    CPF_ExportObject) &&
                Property->HasAllPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
            {
                Out.Add(Property->GetFName());
            }
        }
    }
}

void UBtf_TaskForge::CollectFunctions(const UClass* InClass, TSet<FName>& Out)
{
    Out.Reset();
    for (TFieldIterator<UField> It(InClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        if (const auto* LocFunction = Cast<UFunction>(*It))
        {
            if (LocFunction->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public) &&
                NOT LocFunction->GetBoolMetaData(FName(TEXT("BlueprintInternalUseOnly"))) &&
                NOT LocFunction->GetBoolMetaData(FName(TEXT("ExposeAutoCall"))) &&
                NOT LocFunction->HasMetaData(FName(TEXT("DeprecatedFunction"))) &&
                NOT FObjectEditorUtils::IsFunctionHiddenFromClass(LocFunction, InClass) &&
                NOT LocFunction->HasAnyFunctionFlags(
                    FUNC_Static |
                    FUNC_UbergraphFunction |
                    FUNC_Delegate |
                    FUNC_Private |
                    FUNC_Protected |
                    FUNC_EditorOnly |
                    FUNC_BlueprintPure |
                    FUNC_Const))
            {
                Out.Add(LocFunction->GetFName());
            }
        }
    }
}

void UBtf_TaskForge::CollectDelegates(const UClass* InClass, TSet<FName>& Out)
{
    Out.Reset();
    for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
    {
        if (const auto* DelegateProperty = CastField<FMulticastDelegateProperty>(*PropertyIt))
        {
            if (DelegateProperty->HasAnyPropertyFlags(FUNC_Private | CPF_Protected | FUNC_EditorOnly) ||
                NOT DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintAssignable) ||
                DelegateProperty->GetBoolMetaData(FName(TEXT("BlueprintInternalUseOnly"))) ||
                DelegateProperty->HasMetaData(TEXT("DeprecatedFunction")))
            {
                continue;
            }

            if (const auto LocFunction = DelegateProperty->SignatureFunction)
            {
                if (NOT LocFunction->HasAllFunctionFlags(FUNC_Public) ||
                    LocFunction->GetBoolMetaData(TEXT("BlueprintInternalUseOnly")) ||
                    LocFunction->HasMetaData(TEXT("DeprecatedFunction")) ||
                    LocFunction->HasAnyFunctionFlags(
                        FUNC_Static |
                        FUNC_BlueprintPure |
                        FUNC_Const |
                        FUNC_UbergraphFunction |
                        FUNC_Private |
                        FUNC_Protected |
                        FUNC_EditorOnly))
                {
                    continue;
                }
            }
            Out.Add(DelegateProperty->GetFName());
        }
    }
}

void UBtf_TaskForge::CleanInvalidParams(TArray<FBtf_NameSelect>& Arr, const TSet<FName>& ArrRef)
{
    for (int32 i = Arr.Num() - 1; i >= 0; --i)
    {
        if ((Arr[i].Name != NAME_None && NOT ArrRef.Contains(Arr[i])) || Arr[i].Name == GET_MEMBER_NAME_CHECKED(UBtf_TaskForge, OnCustomPinTriggered))
        {
            Arr.RemoveAt(i, 1, EAllowShrinking::No);
        }
    }
}

void UBtf_TaskForge::RefreshCollected()
{
#if WITH_EDITORONLY_DATA
    {
        const auto* InClass = GetClass();
        CollectSpawnParam(InClass, AllParam);
        CollectFunctions(InClass, AllFunctions);
        CollectDelegates(InClass, AllDelegates);
        AllFunctionsExec = AllFunctions;
        CleanInvalidParams(AutoCallFunction, AllFunctions);
        CleanInvalidParams(ExecFunction, AllFunctions);
        CleanInvalidParams(InDelegate, AllDelegates);
        CleanInvalidParams(OutDelegate, AllDelegates);
        CleanInvalidParams(SpawnParam, AllParam);
        AutoCallFunction.AddUnique(FBtf_NameSelect{TEXT("Activate")});
        for (auto& It : AutoCallFunction)
        {
            It.SetAllExclude(AllFunctions, AutoCallFunction);
            ExecFunction.Remove(It);
        }
        for (auto& It : ExecFunction)
        {
            It.SetAllExclude(AllFunctionsExec, ExecFunction);
            AutoCallFunction.Remove(It);
        }
        for (auto& It : InDelegate)
        {
            It.SetAllExclude(AllDelegates, InDelegate);
            OutDelegate.Remove(It);
        }
        for (auto& It : OutDelegate)
        {
            It.SetAllExclude(AllDelegates, OutDelegate);
            InDelegate.Remove(It);
        }
        for (auto& It : SpawnParam)
        {
            It.SetAllExclude(AllParam, SpawnParam);
        }
        AutoCallFunction.AddUnique(FBtf_NameSelect{TEXT("Activate")});

        AutoCallFunction.Remove(FBtf_NameSelect{TEXT("Deactivate")});

        if (const auto* DeveloperSettings = GetDefault<UBtf_RuntimeSettings>())
        {
            if (DeveloperSettings->EnforceDeactivateExecFunction)
            {
                ExecFunction.AddUnique(FBtf_NameSelect{TEXT("Deactivate")});
            }
        }
    }
#endif
}

void UBtf_TaskForge::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    RefreshCollected();
    Super::PostEditChangeProperty(PropertyChangedEvent);

    OnPostPropertyChanged.ExecuteIfBound(PropertyChangedEvent);
}
#endif

// --------------------------------------------------------------------------------------------------------------------