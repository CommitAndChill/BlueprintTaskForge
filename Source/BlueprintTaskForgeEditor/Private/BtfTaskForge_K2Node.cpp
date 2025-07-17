// Copyright (c) 2025 BlueprintTaskForge Maintainers
//
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
//
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
//
// SPDX-License-Identifier: BTFPL-1.0

#include "BtfTaskForge_K2Node.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "BtfTriggerCustomOutputPin_K2Node.h"

#include "Kismet2/BlueprintEditorUtils.h"

#include "BlueprintTaskForge/Public/BtfTaskForge.h"
#include "BlueprintTaskForge/Public/Subsystem/BtfSubsystem.h"
#include "Settings/BtfRuntimeSettings.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "K2Node"

UBtf_TaskForge_K2Node::UBtf_TaskForge_K2Node(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UBtf_TaskForge, BlueprintTaskForge);
    ProxyFactoryClass = UBtf_TaskForge::StaticClass();
    OutPutObjectPinName = FName(TEXT("TaskObject"));
}

void UBtf_TaskForge_K2Node::PinDefaultValueChanged(UEdGraphPin* Pin)
{
    /* Without this check, the @ReconstructNode would somehow
     * lead to a chain of events that would mark the parent blueprint
     * as dirty as the editor is loading, leading to some blueprints
     * always being labelled as dirty. This check prevents that. */
    if (Decorator.IsValid())
    {
        /* If the node is using a decorator, we want to refresh the node in case the decorator
         * is reading the data from the input pins to drive its visuals.
         * If we instantly reconstruct the node, then we encounter a race condition when we drag
         * off the pin and create a variable and cause a crash. Using this timer gets around that
         * that race condition. */
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float DeltaTime) -> bool
        {
            ReconstructNode();
            return false;
        }));
    }
}

void UBtf_TaskForge_K2Node::HideClassPin() const
{
    auto* ClassPin = FindPinChecked(ClassPinName);
    ClassPin->DefaultObject = ProxyClass;
    ClassPin->DefaultValue.Empty();
    ClassPin->bDefaultValueIsReadOnly = true;
    ClassPin->bNotConnectable = true;
    ClassPin->bHidden = true;
}

void UBtf_TaskForge_K2Node::RegisterBlueprintAction(UClass* TargetClass, FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    const auto Name = TargetClass->GetName();

    if (NOT TargetClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated) &&
        NOT Name.Contains(FNames_Helper::SkelPrefix) &&
        NOT Name.Contains(FNames_Helper::ReinstPrefix) &&
        NOT Name.Contains(FNames_Helper::DeadclassPrefix))
    {
        auto* NodeClass = GetClass();
        auto Lambda = [NodeClass, TargetClass](const UFunction* FactoryFunc) -> UBlueprintNodeSpawner*
        {
            auto CustomizeTimelineNodeLambda = [TargetClass](UEdGraphNode* NewNode, bool IsTemplateNode, const TWeakObjectPtr<UFunction> FunctionPtr)
            {
                auto* AsyncTaskNode = CastChecked<UBtf_TaskForge_K2Node>(NewNode);
                if (FunctionPtr.IsValid())
                {
                    auto* Func = FunctionPtr.Get();
                    AsyncTaskNode->ProxyFactoryFunctionName = Func->GetFName();
                    AsyncTaskNode->ProxyClass = TargetClass;
                }
            };

            auto* NodeSpawner = UBlueprintFunctionNodeSpawner::Create(FactoryFunc);
            NodeSpawner->NodeClass = NodeClass;
            if (IsValid(TargetClass) && TargetClass->HasMetaData(TEXT("Category")))
            {
                NodeSpawner->DefaultMenuSignature.Category = FText::FromString(TargetClass->GetMetaData(TEXT("Category")));
            }

            const auto FunctionPtr = TWeakObjectPtr<UFunction>(const_cast<UFunction*>(FactoryFunc));
            auto& MenuSignature = NodeSpawner->DefaultMenuSignature;

            auto LocName = TargetClass->GetName();
            LocName.RemoveFromEnd(FNames_Helper::CompiledFromBlueprintSuffix);

            if (const auto* TargetClassAsBlueprintTask = GetDefault<UBtf_TaskForge>(TargetClass))
            {
                if (TargetClassAsBlueprintTask->Category != NAME_None)
                {
                    MenuSignature.Category = FText::FromName(TargetClassAsBlueprintTask->Category);
                }

                if (TargetClassAsBlueprintTask->Tooltip != NAME_None)
                {
                    MenuSignature.Tooltip = FText::FromName(TargetClassAsBlueprintTask->Tooltip);
                }

                MenuSignature.MenuName = TargetClassAsBlueprintTask->MenuDisplayName != NAME_None
                                            ? FText::FromName(TargetClassAsBlueprintTask->MenuDisplayName)
                                            : FText::FromString(LocName);

                MenuSignature.Keywords = MenuSignature.MenuName;
            }
            else
            {
                MenuSignature.MenuName = FText::FromString(LocName);
                MenuSignature.Keywords = FText::FromString(LocName);
            }

            NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateLambda(CustomizeTimelineNodeLambda, FunctionPtr);
            return NodeSpawner;
        };

        {
            for (TFieldIterator<UFunction> FuncIt(TargetClass); FuncIt; ++FuncIt)
            {
                const auto* const Function = *FuncIt;
                if (NOT Function->HasAnyFunctionFlags(FUNC_Static))
                { continue; }

                if (NOT CastField<FObjectProperty>(Function->GetReturnProperty()))
                { continue; }

                if (auto* NewAction = Lambda(Function))
                {
                    ActionRegistrar.AddBlueprintAction(Function, NewAction);
                }
            }
        }
    }
}

void UBtf_TaskForge_K2Node::AllocateDefaultPins()
{
    Super::AllocateDefaultPins();
    check(GetWorldContextPin());
    HideClassPin();

    FindPin(NodeGuidStrName)->DefaultValue = ProxyClass->GetName() + NodeGuid.ToString();
    FindPin(NodeGuidStrName)->bHidden = true;
    FindPin(NodeGuidStrName)->Modify();

    UK2Node::AllocateDefaultPins();
    GetGraph()->NotifyGraphChanged();
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UBtf_TaskForge_K2Node::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);
    check(GetWorldContextPin());
    HideClassPin();

    UK2Node::AllocateDefaultPins();
    GetGraph()->NotifyGraphChanged();
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UBtf_TaskForge_K2Node::ReconstructNode()
{
    if (const auto* TargetClassAsBlueprintTask = GetDefault<UBtf_TaskForge>(ProxyClass);
            IsValid(TargetClassAsBlueprintTask))
    {
        if (ProxyClass && ProxyClass->ClassGeneratedBy)
        {
            if (UBlueprint* SourceBlueprint = Cast<UBlueprint>(ProxyClass->ClassGeneratedBy))
            {
                // Find all TriggerCustomOutputPin nodes
                for (UEdGraphNode* Node : SourceBlueprint->UbergraphPages[0]->Nodes)
                {
                    if (UBtf_K2Node_TriggerCustomOutputPin* TriggerNode = Cast<UBtf_K2Node_TriggerCustomOutputPin>(Node);
                        IsValid(TriggerNode))
                    {
                        TriggerNode->CachedCustomPins = CustomPins;
                        TriggerNode->HasValidCachedPins = true;

                        TriggerNode->ReconstructNode();
                    }
            }
            }
        }
    }

    Super::ReconstructNode();
}

void UBtf_TaskForge_K2Node::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    for (TObjectIterator<UClass> It; It; ++It)
    {
        auto* TargetClass = *It;
        if (TargetClass->IsChildOf(ProxyFactoryClass))
        {
            RegisterBlueprintAction(TargetClass, ActionRegistrar);
        }
    }
}

void UBtf_TaskForge_K2Node::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);
}

FText UBtf_TaskForge_K2Node::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (IsValid(ProxyClass))
    {
        if (const auto* TargetClassAsBlueprintTask = GetDefault<UBtf_TaskForge>(ProxyClass);
            IsValid(TargetClassAsBlueprintTask))
        {
            const auto& MenuDisplayName = TargetClassAsBlueprintTask->MenuDisplayName;
            if (MenuDisplayName != NAME_None)
            { return FText::FromName(MenuDisplayName); }
        }

        const auto Str = ProxyClass->GetName();
        auto ParseNames = TArray<FString>{};
        Str.ParseIntoArray(ParseNames, *FNames_Helper::CompiledFromBlueprintSuffix);
        return FText::FromString(ParseNames[0]);
    }
    return FText(LOCTEXT("UBtf_TaskForge_K2Node", "Node_Blueprint_Task Function"));
}

FText UBtf_TaskForge_K2Node::GetMenuCategory() const
{
    if (IsValid(ProxyClass) && ProxyClass->HasMetaData(TEXT("Category")))
    {
        return FText::FromString(ProxyClass->GetMetaData(TEXT("Category")));
    }
    return Super::GetMenuCategory();
}

FString UBtf_TaskForge_K2Node::Get_NodeDescription() const
{
    if (NOT IsValid(GEditor->PlayWorld) && IsValid(ProxyClass))
    {
        if (IsValid(TaskInstance))
        {
            return TaskInstance->Get_NodeDescription();
        }

        if (const auto& TaskCDO = GetDefault<UBtf_TaskForge>(ProxyClass);
            IsValid(TaskCDO))
        {
            const auto& NodeDescription = TaskCDO->Get_NodeDescription();
            return NodeDescription;
        }

        return {};
    }

    if (const auto* Settings = GetDefault<UBtf_RuntimeSettings>())
    {

      if (const auto ShowNodeDescriptionWhilePlaying = Settings->ShowNodeDescriptionWhilePlaying;
          NOT ShowNodeDescriptionWhilePlaying)
      { return {}; }

      if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
          IsValid(Subsystem))
      {
          if (const auto FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
              IsValid(FoundTaskInstance))
          {
              const auto& NodeDescription = FoundTaskInstance->Get_NodeDescription();
              return NodeDescription;
          }
      }
    }

    return {};
}

FString UBtf_TaskForge_K2Node::Get_StatusString() const
{
    if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
        IsValid(Subsystem))
    {
        if (const auto FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
            IsValid(FoundTaskInstance))
        {
            if (NOT FoundTaskInstance->Get_IsActive())
            { return {}; }

            const auto& NodeStatus = FoundTaskInstance->Get_StatusString();
            return NodeStatus;
        }
    }

    return {};
}

FLinearColor UBtf_TaskForge_K2Node::Get_StatusBackgroundColor() const
{
    auto ObtainedColor = FLinearColor{};
    if (NOT IsValid(GEditor->PlayWorld) && IsValid(ProxyClass))
    {
        if (IsValid(TaskInstance))
        {
            if (TaskInstance->Get_StatusBackgroundColor(ObtainedColor))
            {
                return ObtainedColor;
            }
        }

        if (const auto& TaskCDO = GetDefault<UBtf_TaskForge>(ProxyClass);
            IsValid(TaskCDO))
        {
            if (TaskCDO->Get_StatusBackgroundColor(ObtainedColor))
            {
                return ObtainedColor;
            }
        }

        return {};
    }

    if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
        IsValid(Subsystem))
    {
        if (const auto FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
            IsValid(FoundTaskInstance))
        {
            if (FoundTaskInstance->Get_StatusBackgroundColor(ObtainedColor))
            {
                return ObtainedColor;
            }
        }
    }

    constexpr auto NodeStatusBackground = FLinearColor(0.12f, 0.12f, 0.12f, 1.0f);
    return NodeStatusBackground;
}

FText UBtf_TaskForge_K2Node::Get_NodeConfigText() const
{
    if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
        IsValid(Subsystem))
    {
        if (const auto FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
            IsValid(FoundTaskInstance))
        {
            if (FoundTaskInstance->Get_IsActive())
            {
                return FText::FromName(TEXT("Running..."));
            }
        }
    }

    return FText::GetEmpty();
}

TSet<FName> UBtf_TaskForge_K2Node::Get_PinsHiddenByDefault()
{
    auto PinsHiddenByDefault = TSet{Super::Get_PinsHiddenByDefault()};
    PinsHiddenByDefault.Add(TEXT("NodeGuidStr"));

    return PinsHiddenByDefault;
}

#if WITH_EDITORONLY_DATA
void UBtf_TaskForge_K2Node::ResetToDefaultExposeOptions_Impl()
{
    if (IsValid(ProxyClass))
    {
        if (const auto CDO = ProxyClass->GetDefaultObject<UBtf_TaskForge>())
        {
            AllDelegates = CDO->AllDelegates;
            AllFunctions = CDO->AllFunctions;
            AllFunctionsExec = CDO->AllFunctionsExec;
            AllParam = CDO->AllParam;

            SpawnParam = CDO->SpawnParam;
            AutoCallFunction = CDO->AutoCallFunction;
            ExecFunction = CDO->ExecFunction;
            InDelegate = CDO->InDelegate;
            OutDelegate = CDO->OutDelegate;
        }
    }
    ReconstructNode();
}
#endif

void UBtf_TaskForge_K2Node::CollectSpawnParam(UClass* InClass, const bool FullRefresh)
{
    if (IsValid(InClass))
    {
        if (const auto* CDO = InClass->GetDefaultObject<UBtf_TaskForge>())
        {
            AllDelegates = CDO->AllDelegates;
            AllFunctions = CDO->AllFunctions;
            AllFunctionsExec = CDO->AllFunctionsExec;
            AllParam = CDO->AllParam;
            if (FullRefresh)
            {
                SpawnParam = CDO->SpawnParam;
                AutoCallFunction = CDO->AutoCallFunction;
                ExecFunction = CDO->ExecFunction;
                InDelegate = CDO->InDelegate;
                OutDelegate = CDO->OutDelegate;
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------