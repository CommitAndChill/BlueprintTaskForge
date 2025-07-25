// Copyright (c) 2025 BlueprintTaskForge Maintainers
//
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
//
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
//
// SPDX-License-Identifier: BTFPL-1.0

#include "BtfExtendConstructObject_K2Node.h"
#include "BtfExtendConstructObject_Utils.h"

#include "Misc/AssertionMacros.h"
#include "Algo/Unique.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/Field.h"
#include "UObject/NoExportTypes.h"
#include "UObject/FrameworkObjectVersion.h"
#include "ObjectEditorUtils.h"
#include "Delegates/DelegateSignatureImpl.inl"

#include "EdGraphSchema_K2.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_TemporaryVariable.h"
#include "K2Node_EnumLiteral.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_AssignDelegate.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Self.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_EditablePinBase.h"

#include "KismetCompiler.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BtfTaskForge.h"
#include "DetailLayoutBuilder.h"
#include "K2Node_BreakStruct.h"

#include "Framework/Commands/UIAction.h"
#include "ToolMenu.h"
#include "K2Node_SwitchName.h"
#include "ObjectTools.h"

#include "Kismet/BlueprintInstancedStructLibrary.h"

#include "NodeCustomizations/BtfGraphNode.h"
#include "StructUtils/InstancedStruct.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "K2Node"

FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::InPrefix = FString::Printf(TEXT("In_"));
FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::OutPrefix = FString::Printf(TEXT("Out_"));
FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::InitPrefix = FString::Printf(TEXT("Init_"));

FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::SkelPrefix = FString::Printf(TEXT("SKEL_"));
FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::ReinstPrefix = FString::Printf(TEXT("REINST_"));
FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::DeadclassPrefix = FString::Printf(TEXT("DEADCLASS_"));

FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::CompiledFromBlueprintSuffix = FString::Printf(TEXT("_C"));

UBtf_ExtendConstructObject_K2Node::UBtf_ExtendConstructObject_K2Node(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ProxyFactoryClass(nullptr)
    , ProxyClass(nullptr)
    , ProxyFactoryFunctionName(NAME_None)
    , PinTooltipsValid(false)
{
    ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UBtf_ExtendConstructObject_Utils, ExtendConstructObject);
    ProxyFactoryClass = UBtf_ExtendConstructObject_Utils::StaticClass();
    WorldContextPinName = FName(TEXT("Outer"));
}

UEdGraphPin* UBtf_ExtendConstructObject_K2Node::GetWorldContextPin() const
{
    if (SelfContext)
    { return FindPin(UEdGraphSchema_K2::PSC_Self); }

    return FindPin(WorldContextPinName);
}

#if WITH_EDITORONLY_DATA
void UBtf_ExtendConstructObject_K2Node::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    auto NeedReconstruct = false;
    const auto PropertyName = PropertyChangedEvent.GetPropertyName();

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, AutoCallFunction))
    {
        RefreshNames(ExecFunction, false);
        for (auto& FuncName : AutoCallFunction)
        {
            FuncName.SetAllExclude(AllFunctions, AutoCallFunction);
            ExecFunction.Remove(FuncName);
        }
        NeedReconstruct = true;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, ExecFunction))
    {
        RefreshNames(ExecFunction, false);
        for (auto& FuncName : ExecFunction)
        {
            FuncName.SetAllExclude(AllFunctionsExec, ExecFunction);
            AutoCallFunction.Remove(FuncName);
        }
        NeedReconstruct = true;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, InDelegate))
    {
        RefreshNames(InDelegate, false);
        for (auto& DelegateName : InDelegate)
        {
            DelegateName.SetAllExclude(AllDelegates, InDelegate);
            OutDelegate.Remove(DelegateName);
        }
        NeedReconstruct = true;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, OutDelegate))
    {
        RefreshNames(OutDelegate, false);
        for (auto& DelegateName : OutDelegate)
        {
            DelegateName.SetAllExclude(AllDelegates, OutDelegate);
            InDelegate.Remove(DelegateName);
        }
        NeedReconstruct = true;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, SpawnParam))
    {
        RefreshNames(SpawnParam, false);
        for (auto& Param : SpawnParam)
        {
            Param.SetAllExclude(AllParam, SpawnParam);
        }
        NeedReconstruct = true;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, OwnerContextPin))
    {
        NeedReconstruct = true;
        if (OwnerContextPin)
        { SelfContext = false; }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, AllowInstance))
    {
        NeedReconstruct = true;

        if (AllowInstance)
        {
            auto InstanceFound = false;
            for (auto& CurrentExtension : GetBlueprint()->GetExtensions())
            {
                if (CurrentExtension.IsA(UBtf_TaskForge::StaticClass()) && CurrentExtension.GetName().Contains(NodeGuid.ToString()))
                {
                    TaskInstance = Cast<UBtf_TaskForge>(CurrentExtension);
                    TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent)
                    {
                        ReconstructNode();
                    });
                    InstanceFound = true;
                }
            }

            if (NOT InstanceFound)
            {
                TaskInstance = NewObject<UBtf_TaskForge>(GetBlueprint(), ProxyClass, GetTemplateInstanceName());
                GetBlueprint()->AddExtension(TaskInstance);
                std::ignore = GetBlueprint()->MarkPackageDirty();
                std::ignore = TaskInstance->MarkPackageDirty();

                TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent)
                {
                    ReconstructNode();
                });
            }
        }
        else
        {
            if (IsValid(TaskInstance))
            {
                TaskInstance->OnPostPropertyChanged.Unbind();
                GetBlueprint()->RemoveExtension(TaskInstance);
                TaskInstance->MarkAsGarbage();
                TaskInstance = nullptr;
            }
        }

        if (DetailsPanelBuilder)
        { DetailsPanelBuilder->ForceRefreshDetails(); }
    }

    if (NeedReconstruct)
    {
        Modify();
        ReconstructNode();
    }

    if (PropertyChangedEvent.Property != nullptr && PropertyChangedEvent.Property->GetFName() == TEXT("CustomPins"))
    { ReconstructNode(); }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UBtf_ExtendConstructObject_K2Node::ResetToDefaultExposeOptions_Impl()
{
    ReconstructNode();
}
#endif

void UBtf_ExtendConstructObject_K2Node::Serialize(FArchive& Ar)
{
    Ar.UsingCustomVersion(FFrameworkObjectVersion::GUID);

    if (Ar.IsSaving())
    {
        if (IsValid(ProxyClass) && NOT ProxyClass->IsChildOf(UObject::StaticClass()))
        { ProxyClass = nullptr; }
    }

    Super::Serialize(Ar);

    if (Ar.IsLoading())
    {
        if (IsValid(ProxyClass) && NOT ProxyClass->IsChildOf(UObject::StaticClass()))
        { ProxyClass = nullptr; }

        if (NOT OwnerContextPin && FindPin(WorldContextPinName))
        {
            SelfContext = false;
            OwnerContextPin = true;
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
    Super::GetNodeContextMenuActions(Menu, Context);
    if (Context->bIsDebugging)
    { return; }

    if (IsValid(Context->Node) && Context->Node->GetClass()->IsChildOf(UBtf_ExtendConstructObject_K2Node::StaticClass()) && Context->Pin == nullptr)
    {
        auto& Section = Menu->AddSection("ExtendNode", LOCTEXT("TemplateNode", "TemplateNode"));

        if (AllParam.Num())
        {
            Section.AddSubMenu(
                FName("Add_SpawnParamPin"),
                FText::FromName(FName("AddSpawnParamPin")),
                FText(),
                FNewToolMenuDelegate::CreateLambda(
                    [&](UToolMenu* SubMenu)
                    {
                        auto& SubMenuSection = SubMenu->AddSection("AddSpawnParamPin", FText::FromName(FName("AddSpawnParamPin")));
                        for (const auto& ParamName : AllParam)
                        {
                            if (NOT SpawnParam.Contains(ParamName))
                            {
                                SubMenuSection.AddMenuEntry(
                                    ParamName,
                                    FText::FromName(ParamName),
                                    FText(),
                                    FSlateIcon(),
                                    FUIAction(
                                        FExecuteAction::CreateUObject(
                                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                                            &UBtf_ExtendConstructObject_K2Node::AddSpawnParam,
                                            ParamName),
                                        FIsActionChecked()));
                            }
                        }
                    }));
        }

        if (AllFunctions.Num())
        {
            Section.AddSubMenu(
                FName("AddAutoCallFunction"),
                FText::FromName(FName("AddAutoCallFunction")),
                FText(),
                FNewToolMenuDelegate::CreateLambda(
                    [&](UToolMenu* SubMenu)
                    {
                        auto& SubMenuSection = SubMenu->AddSection("AddAutoCallFunction", LOCTEXT("AddAutoCallFunction", "AddAutoCallFunction"));
                        for (const auto& FuncName : AllFunctions)
                        {
                            if (NOT AutoCallFunction.Contains(FuncName))
                            {
                                SubMenuSection.AddMenuEntry(
                                    FuncName,
                                    FText::FromName(FuncName),
                                    FText(),
                                    FSlateIcon(),
                                    FUIAction(
                                        FExecuteAction::CreateUObject(
                                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                                            &UBtf_ExtendConstructObject_K2Node::AddAutoCallFunction,
                                            FuncName),
                                        FIsActionChecked()));
                            }
                        }
                    }));
        }

        if (AllFunctionsExec.Num())
        {
            Section.AddSubMenu(
                FName("AddExecCallFunction"),
                FText::FromName(FName("AddExecCallFunction")),
                FText(),
                FNewToolMenuDelegate::CreateLambda(
                    [&](UToolMenu* SubMenu)
                    {
                        auto& SubMenuSection = SubMenu->AddSection("AddExecCallFunction", LOCTEXT("AddExecCallFunction", "AddCallFunctionPin"));
                        for (const auto& FuncName : AllFunctionsExec)
                        {
                            if (NOT ExecFunction.Contains(FuncName))
                            {
                                SubMenuSection.AddMenuEntry(
                                    FuncName,
                                    FText::FromName(FuncName),
                                    FText(),
                                    FSlateIcon(),
                                    FUIAction(
                                        FExecuteAction::CreateUObject(
                                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                                            &UBtf_ExtendConstructObject_K2Node::AddInputExec,
                                            FuncName),
                                        FIsActionChecked()));
                            }
                        }
                    }));
        }

        if (AllDelegates.Num())
        {
            Section.AddSubMenu(
                FName("Add_InputDelegatePin"),
                FText::FromName(FName("AddInputDelegatePin")),
                FText(),
                FNewToolMenuDelegate::CreateLambda(
                    [&](UToolMenu* SubMenu)
                    {
                        auto& SubMenuSection = SubMenu->AddSection("AddInputDelegatePin", FText::FromName(FName("AddInputDelegatePin")));
                        for (const auto& DelegateName : AllDelegates)
                        {
                            if (NOT InDelegate.Contains(DelegateName))
                            {
                                SubMenuSection.AddMenuEntry(
                                    DelegateName,
                                    FText::FromName(DelegateName),
                                    FText(),
                                    FSlateIcon(),
                                    FUIAction(
                                        FExecuteAction::CreateUObject(
                                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                                            &UBtf_ExtendConstructObject_K2Node::AddInputDelegate,
                                            DelegateName),
                                        FIsActionChecked()));
                            }
                        }
                    }));

            Section.AddSubMenu(
                FName("Add_OutputDelegatePin"),
                FText::FromName(FName("AddOutputDelegatePin")),
                FText(),
                FNewToolMenuDelegate::CreateLambda(
                    [&](UToolMenu* SubMenu)
                    {
                        auto& SubMenuSection = SubMenu->AddSection("SubAsyncTaskSubMenu", FText::FromName(FName("AddOutputDelegatePin")));
                        for (const auto& DelegateName : AllDelegates)
                        {
                            if (NOT OutDelegate.Contains(DelegateName))
                            {
                                SubMenuSection.AddMenuEntry(
                                    DelegateName,
                                    FText::FromName(DelegateName),
                                    FText(),
                                    FSlateIcon(),
                                    FUIAction(
                                        FExecuteAction::CreateUObject(
                                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                                            &UBtf_ExtendConstructObject_K2Node::AddOutputDelegate,
                                            DelegateName),
                                        FIsActionChecked()));
                            }
                        }
                    }));
        }
    }

    if (IsValid(Context->Node) &&
        Context->Node->GetClass()->IsChildOf(UBtf_ExtendConstructObject_K2Node::StaticClass()) &&
        Context->Pin)
    {
        if (Context->Pin->PinName == UEdGraphSchema_K2::PN_Execute || Context->Pin->PinName == UEdGraphSchema_K2::PN_Then)
        { return; }

        const auto SpawnParamPin = SpawnParam.Contains(Context->Pin->PinName);

        if (Context->Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec || Context->Pin->PinType.PinCategory == UEdGraphSchema_K2::PN_EntryPoint ||
            (Context->Pin->Direction == EGPD_Input && Context->Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Delegate) || SpawnParamPin)
        {
            const auto RemPinName = Context->Pin->PinName;
            if (SpawnParamPin ||
                ExecFunction.Contains(RemPinName) ||
                AutoCallFunction.Contains(RemPinName) ||
                InDelegate.Contains(RemPinName) ||
                OutDelegate.Contains(RemPinName))
            {
                auto& Section = Menu->AddSection("ExtendNode", LOCTEXT("ExtendNode", "ExtendNode"));
                Section.AddMenuEntry(
                    FName("RemovePin"),
                    FText::FromName(FName("RemovePin")),
                    FText::GetEmpty(),
                    FSlateIcon(),
                    FUIAction(
                        FExecuteAction::CreateUObject(
                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                            &UBtf_ExtendConstructObject_K2Node::RemoveNodePin,
                            RemPinName),
                        FIsActionChecked()));
            }
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    if (const auto ActionKey = GetClass();
        ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        auto* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);

        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

FText UBtf_ExtendConstructObject_K2Node::GetMenuCategory() const
{
    if (const auto* TargetFunction = GetFactoryFunction())
    { return UK2Node_CallFunction::GetDefaultCategoryForFunction(TargetFunction, FText::GetEmpty()); }

    return FText();
}

bool UBtf_ExtendConstructObject_K2Node::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
    const auto GraphType = TargetGraph->GetSchema()->GetGraphType(TargetGraph);
    const auto IsCompatible = GraphType == EGraphType::GT_Ubergraph || GraphType == EGraphType::GT_Macro || GraphType == EGraphType::GT_Function;
    return IsCompatible && Super::IsCompatibleWithGraph(TargetGraph);
}

void UBtf_ExtendConstructObject_K2Node::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    Super::GetMenuEntries(ContextMenuBuilder);
}

void UBtf_ExtendConstructObject_K2Node::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
    Super::ValidateNodeDuringCompilation(MessageLog);

    if (const auto* SourceObject = MessageLog.FindSourceObject(this))
    {
        if (const auto* MacroInstance = Cast<UK2Node_MacroInstance>(SourceObject))
        {
            if (NOT (GetGraph()->HasAnyFlags(RF_Transient) && GetGraph()->GetName().StartsWith(UEdGraphSchema_K2::FN_ExecuteUbergraphBase.ToString())))
            {
                MessageLog.Error(
                    *LOCTEXT("AsyncTaskInFunctionFromMacro", "@@ is being used in Function '@@' resulting from expansion of Macro '@@'").ToString(),
                    this,
                    GetGraph(),
                    MacroInstance);
            }
        }
    }

    if (NOT IsValid(ProxyClass))
    {
        if (const auto* SourceObject = MessageLog.FindSourceObject(this))
        {
            if (const auto* MacroInstance = Cast<UK2Node_MacroInstance>(SourceObject))
            {
                MessageLog.Error(
                    *LOCTEXT("ExtendConstructObject", "@@ is being used in Function '@@' resulting from expansion of Macro '@@'").ToString(),
                    this,
                    GetGraph(),
                    MacroInstance);
            }
        }
    }

    if (const auto* ClassPin = FindPin(ClassPinName, EGPD_Input);
        NOT(IsValid(ProxyClass) && ClassPin != nullptr && ClassPin->Direction == EGPD_Input))
    {
        MessageLog.Error(
            *FText::Format(
                 LOCTEXT("GenericCreateObject_WrongClassFmt", "Cannot construct Node Class of type '{0}' in @@"),
                 FText::FromString(GetPathNameSafe(ProxyClass)))
                 .ToString(),
            this);
    }

    if (NOT CanBePlacedInGraph())
    {
        MessageLog.Error(
            *FText::Format(
                 LOCTEXT("ExtendConstructObjectPlacedInGraph", "{0} is not allowed to be placed in this class. @@"),
                 FText::FromString(GetPathNameSafe(ProxyClass))).ToString(), this);
    }

    if (auto* Task = GetInstanceOrDefaultObject())
    {
        const auto Errors = Task->ValidateNodeDuringCompilation();
        for (const auto& Error : Errors)
        {
            MessageLog.Error(
                *FText::Format(
                     LOCTEXT("ExtendConstructObjectPlacedInGraph", "{0} @@"),
                     FText::FromString(Error)).ToString(), this);
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
    if (NOT PinTooltipsValid)
    {
        for (auto* CurrentPin : Pins)
        {
            if (CurrentPin->Direction == EGPD_Input)
            {
                CurrentPin->PinToolTip.Reset();
                GeneratePinTooltip(*CurrentPin);
            }
        }
        PinTooltipsValid = true;
    }
    return Super::GetPinHoverText(Pin, HoverTextOut);
}

void UBtf_ExtendConstructObject_K2Node::EarlyValidation(FCompilerResultsLog& MessageLog) const
{
    Super::EarlyValidation(MessageLog);

    if (NOT IsValid(ProxyClass))
    {
        MessageLog.Error(
            *FText::Format(
                 LOCTEXT("GenericCreateObject_WrongClassFmt", "Cannot construct Node Class of type '{0}' in @@"),
                 FText::FromString(GetPathNameSafe(ProxyClass)))
                 .ToString(),
            this);
    }
}

void UBtf_ExtendConstructObject_K2Node::GeneratePinTooltip(UEdGraphPin& Pin) const
{
    ensure(Pin.GetOwningNode() == this);

    const auto* Schema = GetSchema();
    check(Schema);

    if (const auto* const K2Schema = Cast<const UEdGraphSchema_K2>(Schema);
        NOT IsValid(K2Schema))
    {
        Schema->ConstructBasicPinTooltip(Pin, FText::GetEmpty(), Pin.PinToolTip);
        return;
    }

    const auto* Function = GetFactoryFunction();
    if (NOT IsValid(Function))
    {
        Schema->ConstructBasicPinTooltip(Pin, FText::GetEmpty(), Pin.PinToolTip);
        return;
    }

    UK2Node_CallFunction::GeneratePinTooltipFromFunction(Pin, Function);
}

bool UBtf_ExtendConstructObject_K2Node::HasExternalDependencies(TArray<UStruct*>* OptionalOutput) const
{
    const auto* SourceBlueprint = GetBlueprint();

    const auto ProxyFactoryResult = (IsValid(ProxyFactoryClass)) && (ProxyFactoryClass->ClassGeneratedBy != SourceBlueprint);
    if (ProxyFactoryResult && OptionalOutput != nullptr)
    { OptionalOutput->AddUnique(ProxyFactoryClass); }

    const auto ProxyResult = (IsValid(ProxyClass)) && (ProxyClass->ClassGeneratedBy != SourceBlueprint);
    if (ProxyResult && OptionalOutput != nullptr)
    { OptionalOutput->AddUnique(ProxyClass); }

    const auto SuperResult = Super::HasExternalDependencies(OptionalOutput);

    return SuperResult || ProxyFactoryResult || ProxyResult;
}

FText UBtf_ExtendConstructObject_K2Node::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (IsValid(ProxyClass))
    {
        auto ClassName = ProxyClass->GetName();
        ClassName.RemoveFromEnd(FNames_Helper::CompiledFromBlueprintSuffix, ESearchCase::Type::CaseSensitive);
        return FText::FromString(FString::Printf(TEXT("%s"), *ClassName));
    }
    return FText(LOCTEXT("UBtf_ExtendConstructObject_K2Node", "ExtendConstructObject"));
}

FText UBtf_ExtendConstructObject_K2Node::GetTooltipText() const
{
    const auto FunctionToolTipText = ObjectTools::GetDefaultTooltipForFunction(GetFactoryFunction());
    return FText::FromString(FunctionToolTipText);
}

FLinearColor UBtf_ExtendConstructObject_K2Node::GetNodeTitleColor() const
{
    auto CustomColor = FLinearColor{};
    if (const auto& TaskObject = GetInstanceOrDefaultObject();
    	IsValid(TaskObject) &&
    	TaskObject->Get_NodeTitleColor(CustomColor))
    { return CustomColor; }

    return Super::GetNodeTitleColor();
}

FName UBtf_ExtendConstructObject_K2Node::GetCornerIcon() const
{
    if (IsValid(ProxyClass))
    {
        if (OutDelegate.Num() == 0)
        { return FName(); }
    }
    return TEXT("Graph.Latent.LatentIcon");
}

bool UBtf_ExtendConstructObject_K2Node::UseWorldContext() const
{
    const auto* BP = GetBlueprint();
    const auto ParentClass = IsValid(BP) ? BP->ParentClass : nullptr;
    return IsValid(ParentClass) ? ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin) != nullptr : false;
}

FString UBtf_ExtendConstructObject_K2Node::GetPinMetaData(const FName PinName, const FName Key)
{
    if (const auto* Metadata = PinMetadataMap.Find(PinName))
    {
        if (const auto* Value = Metadata->Find(Key))
        { return *Value; }
    }

    return Super::GetPinMetaData(PinName, Key);
}

void UBtf_ExtendConstructObject_K2Node::ReconstructNode()
{
    Super::ReconstructNode();
}

void UBtf_ExtendConstructObject_K2Node::PostReconstructNode()
{
    Super::PostReconstructNode();
}

void UBtf_ExtendConstructObject_K2Node::PostPasteNode()
{
    RefreshNames(AutoCallFunction);
    for (auto& FuncName : AutoCallFunction)
    {
        FuncName.SetAllExclude(AllFunctions, AutoCallFunction);
        ExecFunction.Remove(FuncName);
    }
    RefreshNames(ExecFunction);
    for (auto& FuncName : ExecFunction)
    {
        FuncName.SetAllExclude(AllFunctionsExec, ExecFunction);
    }
    RefreshNames(InDelegate);
    for (auto& DelegateName : InDelegate)
    {
        DelegateName.SetAllExclude(AllDelegates, InDelegate);
        OutDelegate.Remove(DelegateName);
    }
    RefreshNames(OutDelegate);
    for (auto& DelegateName : OutDelegate)
    {
        DelegateName.SetAllExclude(AllDelegates, OutDelegate);
    }

    AllowInstance = false;
    TaskInstance = nullptr;

    Super::PostPasteNode();
}

UObject* UBtf_ExtendConstructObject_K2Node::GetJumpTargetForDoubleClick() const
{
    if (IsValid(ProxyClass))
    { return ProxyClass->ClassGeneratedBy; }

    return nullptr;
}

bool UBtf_ExtendConstructObject_K2Node::CanBePlacedInGraph() const
{
    if (const auto* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(GetGraph()))
    {
        if (IsValid(ProxyClass) && IsValid(GetDefault<UBtf_TaskForge>(ProxyClass)) && GetDefault<UBtf_TaskForge>(ProxyClass)->ClassLimitations.IsValidIndex(0))
        {
            for (const auto& CurrentClass : GetDefault<UBtf_TaskForge>(ProxyClass)->ClassLimitations)
            {
                if (NOT CurrentClass.IsNull())
                {
                    if (CurrentClass->GetClassPathName().ToString() == Blueprint->GetPathName() + "_C")
                    { return true; }
                }
            }

            return false;
        }
    }

    return true;
}

UBtf_TaskForge* UBtf_ExtendConstructObject_K2Node::GetInstanceOrDefaultObject() const
{
	if (NOT IsValid(ProxyClass))
	{ return {}; }

    return AllowInstance && IsValid(TaskInstance) ? TaskInstance : ProxyClass->GetDefaultObject<UBtf_TaskForge>();
}

void UBtf_ExtendConstructObject_K2Node::GenerateCustomOutputPins()
{
    if (IsValid(ProxyClass))
    {
        if (auto* TargetClassAsBlueprintTask = GetInstanceOrDefaultObject())
        {
            const auto OldCustomPins = CustomPins;
            CustomPins = TargetClassAsBlueprintTask->Get_CustomOutputPins();

            const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();

            for (const auto& Pin : CustomPins)
            {
                const FString PinNameStr = Pin.PinName;

                if (NOT FindPin(PinNameStr))
                {
                    auto* ExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, FName(PinNameStr));
                    ExecPin->PinToolTip = Pin.Tooltip;
                }

                if (Pin.PayloadType)
                {
                    for (TFieldIterator<FProperty> PropertyIt(Pin.PayloadType); PropertyIt; ++PropertyIt)
                    {
                        if (const auto* Property = *PropertyIt;
                            NOT Property->HasAnyPropertyFlags(CPF_Parm) &&
                            Property->HasAllPropertyFlags(CPF_BlueprintVisible))
                        {
                            const FName PayloadPinName = Property->GetFName();

                            if (NOT FindPin(PayloadPinName))
                            {
                                if (FEdGraphPinType PinType;
                                    K2Schema->ConvertPropertyToPinType(Property, PinType))
                                {
                                    auto* PayloadPin = CreatePin(EGPD_Output, PinType, PayloadPinName);
                                    PayloadPin->PinFriendlyName = Property->GetDisplayNameText();

                                    K2Schema->ConstructBasicPinTooltip(*PayloadPin,
                                        Property->GetToolTipText(), PayloadPin->PinToolTip);
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (const auto& OldPin : OldCustomPins)
                    {
                        if (OldPin.PinName == Pin.PinName)
                        {
                            TArray<UEdGraphPin*> PinsToRemove;
                            for (auto* ExistingPin : Pins)
                            {
                                if (const FString ExistingPinName = ExistingPin->PinName.ToString();
                                    ExistingPinName.StartsWith(PinNameStr + TEXT("_")))
                                { PinsToRemove.Add(ExistingPin); }
                            }

                            for (auto* PinToRemove : PinsToRemove)
                            {
                                RemovePin(PinToRemove);
                                PinToRemove->MarkAsGarbage();
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

TSharedPtr<SGraphNode> UBtf_ExtendConstructObject_K2Node::CreateVisualWidget()
{
    if (IsValid(TaskInstance))
    {
        TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent)
        {
            ReconstructNode();
        });
    }

	if (NOT IsValid(ProxyClass))
	{ return {}; }

    return SNew(SBtf_Node, this, ProxyClass);
}

void UBtf_ExtendConstructObject_K2Node::DestroyNode()
{
    Super::DestroyNode();

    if (IsValid(TaskInstance))
    {
        TaskInstance->OnPostPropertyChanged.Unbind();
        GetBlueprint()->RemoveExtension(TaskInstance);
        TaskInstance->MarkAsGarbage();
        TaskInstance = nullptr;
    }
}

void UBtf_ExtendConstructObject_K2Node::PinDefaultValueChanged(UEdGraphPin* Pin)
{
    if (Pin->PinName == ClassPinName)
    {
        if (Pin && Pin->LinkedTo.Num() == 0)
        { ProxyClass = Cast<UClass>(Pin->DefaultObject); }
        else if (Pin && (Pin->LinkedTo.Num() == 1))
        {
            const auto* SourcePin = Pin->LinkedTo[0];
            ProxyClass = SourcePin ? Cast<UClass>(SourcePin->PinType.PinSubCategoryObject.Get()) : nullptr;
        }
        Modify();
        ErrorMsg.Reset();

        for (int32 PinIndex = Pins.Num() - 1; PinIndex >= 0; --PinIndex)
        {
            const auto PinName = Pins[PinIndex]->PinName;

            if (PinName != UEdGraphSchema_K2::PN_Self &&
                PinName != WorldContextPinName &&
                PinName != OutPutObjectPinName &&
                PinName != ClassPinName &&
                PinName != UEdGraphSchema_K2::PN_Execute &&
                PinName != UEdGraphSchema_K2::PN_Then &&
                PinName != NodeGuidStrName)
            {
                Pins[PinIndex]->MarkAsGarbage();
                Pins.RemoveAt(PinIndex);
            }
        }

        AllocateDefaultPins();
    }
    Super::PinDefaultValueChanged(Pin);
}

UK2Node::ERedirectType UBtf_ExtendConstructObject_K2Node::DoPinsMatchForReconstruction(
    const UEdGraphPin* NewPin,
    const int32 NewPinIndex,
    const UEdGraphPin* OldPin,
    const int32 OldPinIndex) const
{
    return Super::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
}

void UBtf_ExtendConstructObject_K2Node::AllocateDefaultPins()
{
    if (const auto OldPin = FindPin(UEdGraphSchema_K2::PN_Execute))
    {
        const auto Index = Pins.IndexOfByKey(OldPin);
        if (Index != 0)
        {
            Pins.RemoveAt(Index, 1, EAllowShrinking::No);
            Pins.Insert(OldPin, 0);
        }
    }
    else
    { CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute); }

    if (const auto OldPin = FindPin(UEdGraphSchema_K2::PN_Then))
    {
        if (const auto Index = Pins.IndexOfByKey(OldPin);
            Index != 1)
        {
            Pins.RemoveAt(Index, 1, EAllowShrinking::No);
            Pins.Insert(OldPin, 1);
        }
    }
    else
    { CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then); }

    if (IsValid(ProxyClass))
    {
        const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
        if (const auto* Function = GetFactoryFunction())
        {
            for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
            {
                const auto* Param = *PropertyIt;

                const auto IsFunctionOutput = Param->HasAnyPropertyFlags(CPF_OutParm) &&
                    NOT Param->HasAnyPropertyFlags(CPF_ReferenceParm) &&
                    NOT Param->HasAnyPropertyFlags(CPF_ReturnParm);

                if (IsFunctionOutput)
                {
                    auto* Pin = CreatePin(EGPD_Output, NAME_None, Param->GetFName());
                    K2Schema->ConvertPropertyToPinType(Param, Pin->PinType);
                }
            }
        }
        CreatePinsForClass(ProxyClass, true);
    }
    else
    { CreatePinsForClass(nullptr, true); }

    GenerateCustomOutputPins();

    for (auto& CurrentExtension : GetBlueprint()->GetExtensions())
    {
        if (CurrentExtension.IsA(UBtf_TaskForge::StaticClass()))
        {
            if (CurrentExtension.GetName().Contains(NodeGuid.ToString()))
            {
                TaskInstance = Cast<UBtf_TaskForge>(CurrentExtension);
                TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent)
                {
                    ReconstructNode();
                });
            }
        }
    }

    if (IsValid(TaskInstance))
    {
        FindPin(NodeGuidStrName)->DefaultValue = ProxyClass->GetName() + NodeGuid.ToString();
        FindPin(NodeGuidStrName)->DefaultTextValue = FText::FromString(ProxyClass->GetName() + NodeGuid.ToString());
    }

    auto* Graph = GetGraph();
    Graph->NotifyGraphChanged();

    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UBtf_ExtendConstructObject_K2Node::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    RestoreSplitPins(OldPins);

    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

    if (IsValid(ProxyClass))
    {
        const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
        if (const auto* Function = GetFactoryFunction())
        {
            for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
            {
                const auto* Param = *PropertyIt;
                const auto IsFunctionOutput = Param->HasAnyPropertyFlags(CPF_OutParm) &&
                    NOT Param->HasAnyPropertyFlags(CPF_ReferenceParm) &&
                    NOT Param->HasAnyPropertyFlags(CPF_ReturnParm);
                if (IsFunctionOutput)
                {
                    auto* Pin = CreatePin(EGPD_Output, NAME_None, Param->GetFName());
                    K2Schema->ConvertPropertyToPinType(Param, Pin->PinType);
                }
            }
        }
        CreatePinsForClass(ProxyClass, false);
    }
    else
    { CreatePinsForClass(ProxyClass, true); }

    GenerateCustomOutputPins();

    GetGraph()->NotifyGraphChanged();
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

UEdGraphPin* UBtf_ExtendConstructObject_K2Node::FindParamPin(const FString& ContextName, FName NativePinName, EEdGraphPinDirection Direction) const
{
    const auto ParamName = FName(*FString::Printf(TEXT("%s_%s"), *ContextName, *NativePinName.ToString()));
    return FindPin(ParamName, Direction);
}

UFunction* UBtf_ExtendConstructObject_K2Node::GetFactoryFunction() const
{
    if (NOT IsValid(ProxyFactoryClass))
    {
        UE_LOG(LogBlueprint, Error, TEXT("ProxyFactoryClass null in %s. Was a class deleted or saved on a non promoted build?"), *GetFullName());
        return nullptr;
    }

    auto FunctionRef = FMemberReference{};
    FunctionRef.SetExternalMember(ProxyFactoryFunctionName, ProxyFactoryClass);

    auto* FactoryFunction = FunctionRef.ResolveMember<UFunction>(GetBlueprint());

    if (NOT IsValid(FactoryFunction))
    {
        FactoryFunction = ProxyFactoryClass->FindFunctionByName(ProxyFactoryFunctionName);
        UE_CLOG(
            NOT IsValid(FactoryFunction),
            LogBlueprint,
            Error,
            TEXT("FactoryFunction %s null in %s. Was a class deleted or saved on a non promoted build?"),
            *ProxyFactoryFunctionName.ToString(),
            *GetFullName());
    }

    return FactoryFunction;
}

void UBtf_ExtendConstructObject_K2Node::RefreshNames(TArray<FBtf_NameSelect>& NameArray, const bool RemoveNone) const
{
    if (RemoveNone)
    { NameArray.RemoveAll([](const FBtf_NameSelect& Item) { return Item.Name == NAME_None; }); }

    auto CopyNameArray = NameArray;

    CopyNameArray.Sort([](const FBtf_NameSelect& A, const FBtf_NameSelect& B) { return GetTypeHash(A.Name) < GetTypeHash(B.Name); });

    auto UniquePred = [&NameArray](const FBtf_NameSelect& A, const FBtf_NameSelect& B)
    {
        if (A.Name != NAME_None && A.Name == B.Name)
        {
            const auto ID = NameArray.FindLast(FBtf_NameSelect{B.Name});
            NameArray.RemoveAt(ID, 1, EAllowShrinking::No);
            return true;
        }
        return false;
    };

    std::ignore = Algo::Unique(CopyNameArray, UniquePred);
}

void UBtf_ExtendConstructObject_K2Node::ResetPinByNames(TSet<FName>& NameArray)
{
    for (const auto OldPinReference : NameArray)
    {
        if (auto* OldPin = FindPin(OldPinReference))
        {
            Pins.Remove(OldPin);
            OldPin->MarkAsGarbage();
        }
    }
    NameArray.Reset();
}

void UBtf_ExtendConstructObject_K2Node::ResetPinByNames(TArray<FBtf_NameSelect>& NameArray)
{
    for (const auto OldPinReference : NameArray)
    {
        if (auto* OldPin = FindPin(OldPinReference))
        {
            Pins.Remove(OldPin);
            OldPin->MarkAsGarbage();
        }
    }
    NameArray.Reset();
}

void UBtf_ExtendConstructObject_K2Node::AddAutoCallFunction(const FName PinName)
{
    const auto OldNum = AutoCallFunction.Num();
    const auto PinNameSelect = FBtf_NameSelect{PinName};

    AutoCallFunction.AddUnique(PinNameSelect);
    if (OldNum - AutoCallFunction.Num() != 0)
    {
        if (NOT ExecFunction.Remove(PinNameSelect))
        {
            if (auto* Pin = FindPin(PinNameSelect))
            {
                RemovePin(Pin);
                Pin->MarkAsGarbage();
            }
        }
        ReconstructNode();
    }
}

void UBtf_ExtendConstructObject_K2Node::AddInputExec(const FName PinName)
{
    const auto OldNum = ExecFunction.Num();
    const auto PinNameSelect = FBtf_NameSelect{PinName};

    ExecFunction.AddUnique(PinNameSelect);
    if (OldNum - ExecFunction.Num() != 0)
    {
        if (NOT AutoCallFunction.Remove(PinNameSelect))
        {
            if (auto* Pin = FindPin(PinName))
            {
                RemovePin(Pin);
                Pin->MarkAsGarbage();
            }
        }
        ReconstructNode();
    }
}

void UBtf_ExtendConstructObject_K2Node::AddOutputDelegate(const FName PinName)
{
    const auto OldNum = OutDelegate.Num();
    const auto PinNameSelect = FBtf_NameSelect{PinName};

    OutDelegate.AddUnique(PinNameSelect);
    if (OldNum - OutDelegate.Num() != 0)
    {
        if (InDelegate.Remove(PinNameSelect))
        {
            auto* Pin = FindPinChecked(PinNameSelect);
            RemovePin(Pin);
            Pin->MarkAsGarbage();
        }
        ReconstructNode();
    }
}

void UBtf_ExtendConstructObject_K2Node::AddInputDelegate(const FName PinName)
{
    const auto OldNum = InDelegate.Num();
    const auto PinNameSelect = FBtf_NameSelect{PinName};

    InDelegate.AddUnique(PinNameSelect);
    if (OldNum - InDelegate.Num() != 0)
    {
        if (OutDelegate.Remove(PinNameSelect))
        {
            auto* Pin = FindPinChecked(PinName);
            RemovePin(Pin);
            Pin->MarkAsGarbage();
        }
        ReconstructNode();
    }
}

void UBtf_ExtendConstructObject_K2Node::AddSpawnParam(const FName PinName)
{
    const auto OldNum = SpawnParam.Num();
    const auto PinNameSelect = FBtf_NameSelect{PinName};

    SpawnParam.AddUnique(PinNameSelect);
    if (OldNum - SpawnParam.Num() != 0)
    { ReconstructNode(); }
}

void UBtf_ExtendConstructObject_K2Node::RemoveNodePin(const FName PinName)
{
    const auto* Pin = FindPin(PinName);
    const auto PinNameSelect = FBtf_NameSelect{PinName};

    check(Pin);
    if (NOT ExecFunction.Remove(PinNameSelect))
    {
        if (NOT AutoCallFunction.Remove(PinNameSelect))
        {
            if (NOT OutDelegate.Remove(PinNameSelect))
            {
                if (NOT InDelegate.Remove(PinNameSelect))
                { SpawnParam.Remove(PinNameSelect); }
            }
        }
    }

    Modify();
    const auto NameStr = PinName.ToString();
    Pins.RemoveAll(
        [&NameStr](UEdGraphPin* CurrentPin)
        {
            if (CurrentPin->GetName().StartsWith(NameStr))
            {
                CurrentPin->MarkAsGarbage();
                return true;
            }
            return false;
        });

    ReconstructNode();
}

void UBtf_ExtendConstructObject_K2Node::GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const
{
    auto Args = TMap<FString, FStringFormatArg>{};

    if (IsValid(ProxyClass))
    {
        Args.Add(TEXT("ProxyClass"), ProxyClass->GetName());
        Args.Add(TEXT("ProxyClassSeparator"), TEXT("."));
    }
    else
    {
        Args.Add(TEXT("ProxyClass"), TEXT(""));
        Args.Add(TEXT("ProxyClassSeparator"), TEXT(""));
    }
    if (ProxyFactoryFunctionName != NAME_None)
    {
        Args.Add(TEXT("ProxyFactoryFunction"), ProxyFactoryFunctionName.ToString());
        Args.Add(TEXT("ProxyFactoryFunctionSeparator"), TEXT("."));
    }
    else
    {
        Args.Add(TEXT("ProxyFactoryFunction"), TEXT(""));
        Args.Add(TEXT("ProxyFactoryFunctionSeparator"), TEXT(""));
    }

    Args.Add(TEXT("PinName"), Pin.PinName.ToString());

    const auto FullPinName = FString::Format(TEXT("{ProxyClass}{ProxyClassSeparator}{ProxyFactoryFunction}{ProxyFactoryFunctionSeparator}{PinName}"), Args);
    RedirectPinNames.Add(FullPinName);
}

void UBtf_ExtendConstructObject_K2Node::GenerateFactoryFunctionPins(UClass* TargetClass)
{
    const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
    if (const auto* Function = GetFactoryFunction())
    {
        auto AllPinsGood = true;
        auto PinsToHide = Get_PinsHiddenByDefault();

        FBlueprintEditorUtils::GetHiddenPinsForFunction(GetGraph(), Function, PinsToHide);
        for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
        {
            const auto* Property = *PropertyIt;
            const auto IsFunctionInput = NOT Property->HasAnyPropertyFlags(CPF_OutParm) || Property->HasAnyPropertyFlags(CPF_ReferenceParm);
            if (NOT IsFunctionInput)
            { continue; }

            auto PinParams = FCreatePinParams{};
            PinParams.bIsReference = Property->HasAnyPropertyFlags(CPF_ReferenceParm) && IsFunctionInput;

            UEdGraphPin* Pin = nullptr;
            if (const auto PropertyName = Property->GetFName();
                PropertyName == WorldContextPinName || PropertyName == UEdGraphSchema_K2::PSC_Self)
            {
                if (auto* OldPin = FindPin(WorldContextPinName))
                { Pin = OldPin; }

                if (auto* OldPin = FindPin(UEdGraphSchema_K2::PSC_Self))
                { Pin = OldPin; }

                if (NOT OwnerContextPin)
                {
                    if (const auto* ObjectProperty = CastFieldChecked<FObjectProperty>(Property))
                    {
                        const auto BPClass = GetBlueprintClassFromNode();
                        SelfContext = BPClass->IsChildOf(ObjectProperty->PropertyClass);
                        if (SelfContext)
                        {
                            Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UEdGraphSchema_K2::PSC_Self, UEdGraphSchema_K2::PN_Self);
                            Pin->PinFriendlyName = FText::FromName(WorldContextPinName);
                            if (NOT OwnerContextPin && Pin->LinkedTo.Num() == 0)
                            {
                                Pin->BreakAllPinLinks();
                                Pin->bHidden = true;
                            }
                            else
                            {
                                Pin->bHidden = false;
                                K2Schema->ConvertPropertyToPinType(Property, Pin->PinType);
                                K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
                            }
                            continue;
                        }
                        OwnerContextPin = true;
                    }
                }
            }
            else if (PropertyName == ClassPinName)
            {
                if (auto* OldPin = GetClassPin())
                {
                    Pin = OldPin;
                    Pin->bHidden = false;
                }
            }

            if (NOT Pin)
            { Pin = CreatePin(EGPD_Input, NAME_None, Property->GetFName(), PinParams); }
            else
            {
                const auto Index = Pins.IndexOfByKey(Pin);
                Pins.RemoveAt(Index, 1, EAllowShrinking::No);
                Pins.Add(Pin);
            }

            check(Pin);
            const auto PinGood = (Pin && K2Schema->ConvertPropertyToPinType(Property, Pin->PinType));

            if (PinGood)
            {
                Pin->bDefaultValueIsIgnored = Property->HasAllPropertyFlags(CPF_ConstParm | CPF_ReferenceParm) &&
                    (NOT Function->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm) || Pin->PinType.IsContainer());

                const auto AdvancedPin = Property->HasAllPropertyFlags(CPF_AdvancedDisplay);
                Pin->bAdvancedView = AdvancedPin;
                if (AdvancedPin && (ENodeAdvancedPins::NoPins == AdvancedPinDisplay))
                { AdvancedPinDisplay = ENodeAdvancedPins::Hidden; }

                auto ParamValue = FString{};
                if (K2Schema->FindFunctionParameterDefaultValue(Function, Property, ParamValue))
                { K2Schema->SetPinAutogeneratedDefaultValue(Pin, ParamValue); }
                else
                { K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin); }

                if (PinsToHide.Contains(Pin->PinName))
                { Pin->bHidden = true; }
            }

            AllPinsGood = AllPinsGood && PinGood;
        }

        if (AllPinsGood)
        {
            auto* ClassPin = FindPinChecked(ClassPinName);
            ClassPin->DefaultObject = TargetClass;
            ClassPin->DefaultValue.Empty();
            check(GetWorldContextPin());
        }
    }

    if (auto* OldPin = FindPin(OutPutObjectPinName))
    {
        const auto Index = Pins.IndexOfByKey(OldPin);
        Pins.RemoveAt(Index, 1, EAllowShrinking::No);
        Pins.Add(OldPin);
    }
    else
    { CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, TargetClass, OutPutObjectPinName); }
}

void UBtf_ExtendConstructObject_K2Node::GenerateSpawnParamPins(UClass* TargetClass)
{
    PinMetadataMap.Reset();

    const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
    auto* ClassDefaultObject = TargetClass->GetDefaultObject(true);

    for (const auto ParamName : SpawnParam)
    {
        if (ParamName != NAME_None)
        {
            if (const auto* Property = TargetClass->FindPropertyByName(ParamName))
            {
                auto* Pin = CreatePin(EGPD_Input, NAME_None, ParamName);
                check(Pin);
                K2Schema->ConvertPropertyToPinType(Property, Pin->PinType);

				if (ClassDefaultObject)
                {
                    if (const auto* StructProperty = CastField<FStructProperty>(Property))
                    {
	                    if (const uint8* StructData = Property->ContainerPtrToValuePtr<uint8>(ClassDefaultObject))
                        {
                            FString DefaultStructValue;
                            StructProperty->Struct->ExportText(
                                DefaultStructValue,
                                StructData,
                                StructData,
                                ClassDefaultObject,
                                PPF_None,
                                nullptr
                            );
                            K2Schema->TrySetDefaultValue(*Pin, DefaultStructValue);
                        }
                    }
                    else if (K2Schema->PinDefaultValueIsEditable(*Pin))
                    {
                        FString DefaultValueAsString;
                        const bool bDefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(
                            Property,
                            reinterpret_cast<const uint8*>(ClassDefaultObject),
                            DefaultValueAsString,
                            this);
                        if (bDefaultValueSet)
                        {
                            K2Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValueAsString);
                        }
                    }
                }
                K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);

                if (const auto* MetaDataMap = Property->GetMetaDataMap())
                {
                    for (const auto& [MetaKey, MetaValue] : *MetaDataMap)
                    {
                        PinMetadataMap.FindOrAdd(Pin->PinName).Add(MetaKey, MetaValue);
                    }
                }
            }
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GenerateAutoCallFunctionPins(UClass* TargetClass)
{
    for (const auto FunctionName : AutoCallFunction)
    {
        if (FunctionName != NAME_None)
        {
            CreatePinsForClassFunction(TargetClass, FunctionName, true);
            if (auto* ExecPin = FindPin(FunctionName, EGPD_Input))
            {
                ExecPin->bHidden = true;
                ExecPin->bNotConnectable = true;
                ExecPin->bDefaultValueIsReadOnly = true;
                ExecPin->bDefaultValueIsIgnored = true;
                auto& PinType = ExecPin->PinType;
                PinType.PinCategory = UEdGraphSchema_K2::PN_EntryPoint;
            }
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GenerateExecFunctionPins(UClass* TargetClass)
{
    for (const auto FunctionName : ExecFunction)
    {
        if (FunctionName != NAME_None)
        { CreatePinsForClassFunction(TargetClass, FunctionName, false); }
    }
}

void UBtf_ExtendConstructObject_K2Node::GenerateInputDelegatePins(UClass* TargetClass)
{
    for (const auto DelegateName : InDelegate)
    {
        if (DelegateName != NAME_None)
        {
            if (const auto* Property = TargetClass->FindPropertyByName(DelegateName))
            {
                const auto* DelegateProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);

                const auto DelegateNameStr = DelegateName.Name.ToString();

                auto PinParams = FCreatePinParams{};
                PinParams.bIsReference = true;
                PinParams.bIsConst = true;

                if (auto* DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, DelegateName, PinParams))
                {
                    DelegatePin->PinToolTip = DelegateProperty->GetToolTipText().ToString();
                    DelegatePin->PinFriendlyName = FText::FromString(DelegateNameStr);

                    FMemberReference::FillSimpleMemberReference<UFunction>(
                        DelegateProperty->SignatureFunction,
                        DelegatePin->PinType.PinSubCategoryMemberReference);
                }
            }
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GenerateOutputDelegatePins(UClass* TargetClass)
{
    const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
    for (const auto DelegateName : OutDelegate)
    {
        if (DelegateName != NAME_None)
        {
            if (const auto* Property = TargetClass->FindPropertyByName(DelegateName))
            {
                const auto* DelegateProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);
                const auto DelegateNameStr = DelegateName.Name.ToString();

                auto* ExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, DelegateName);
                ExecPin->PinToolTip = DelegateProperty->GetToolTipText().ToString();
                ExecPin->PinFriendlyName = FText::FromString(DelegateNameStr);

                if (const auto DelegateSignatureFunction = DelegateProperty->SignatureFunction)
                {
                    for (TFieldIterator<FProperty> PropertyIt(DelegateSignatureFunction); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
                    {
                        if (const auto* Param = *PropertyIt;
                            NOT Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
                        {
                            const auto ParamFullName = FName(*FString::Printf(TEXT("%s_%s"), *DelegateNameStr, *Param->GetFName().ToString()));
                            auto* Pin = CreatePin(EGPD_Output, NAME_None, ParamFullName);
                            K2Schema->ConvertPropertyToPinType(Param, Pin->PinType);
                            Pin->PinToolTip = ParamFullName.ToString();
                            Pin->PinFriendlyName = FText::FromName(Param->GetFName());
                        }
                    }
                }
            }
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::CreatePinsForClass(UClass* TargetClass, const bool FullRefresh)
{
    if (FullRefresh || NOT IsValid(TargetClass))
    {
        ResetPinByNames(AllParam);
        ResetPinByNames(AllFunctions);
        ResetPinByNames(AllDelegates);
        ResetPinByNames(AllFunctionsExec);

        ResetPinByNames(SpawnParam);
        ResetPinByNames(AutoCallFunction);
        ResetPinByNames(ExecFunction);
        ResetPinByNames(InDelegate);
        ResetPinByNames(OutDelegate);

        auto ContextPin = TSet<FName>{};
        ContextPin.Append({WorldContextPinName, UEdGraphSchema_K2::PSC_Self});
        ResetPinByNames(ContextPin);
    }

    GenerateFactoryFunctionPins(TargetClass);

    if (NOT IsValid(TargetClass))
    { return; }

    const auto* const ClassDefaultObject = TargetClass->GetDefaultObject(true);
    check(ClassDefaultObject);
    CollectSpawnParam(TargetClass, FullRefresh);

    check(ClassDefaultObject);
    GenerateSpawnParamPins(TargetClass);
    GenerateAutoCallFunctionPins(TargetClass);
    GenerateExecFunctionPins(TargetClass);
    GenerateInputDelegatePins(TargetClass);
    GenerateOutputDelegatePins(TargetClass);

    {
        const auto PinNotFoundPredicate = [&](const FBtf_NameSelect& Item) { return Item.Name != NAME_None && NOT FindPin(Item.Name); };
        const auto ValidPropertyPredicate = [TargetClass](const FName& Name)
        {
            if (Name != NAME_None)
            {
                if (NOT TargetClass->FindPropertyByName(Name))
                { return true; }
            }
            return false;
        };
        const auto ValidFunctionPredicate = [TargetClass](const FName& Name)
        {
            if (Name != NAME_None)
            {
                if (NOT TargetClass->FindFunctionByName(Name))
                { return true; }
            }
            return false;
        };

        if (AllParam.Num())
        {
            auto ParamArray = AllParam.Array();
            if (ParamArray.RemoveAll(ValidPropertyPredicate))
            { AllParam = TSet(ParamArray); }

            AllParam.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
            AllParam.Shrink();
            SpawnParam.RemoveAll(PinNotFoundPredicate);
            SpawnParam.Shrink();
        }
        else
        { ResetPinByNames(SpawnParam); }

        if (AllDelegates.Num())
        {
            auto DelegateArray = AllDelegates.Array();
            if (DelegateArray.RemoveAll(ValidPropertyPredicate))
            { AllDelegates = TSet(DelegateArray); }

            AllDelegates.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
            AllDelegates.Shrink();
            InDelegate.RemoveAll(PinNotFoundPredicate);
            OutDelegate.RemoveAll(PinNotFoundPredicate);
            InDelegate.Shrink();
            OutDelegate.Shrink();
        }
        else
        {
            ResetPinByNames(InDelegate);
            ResetPinByNames(OutDelegate);
        }

        if (AllFunctions.Num())
        {
            {
                auto FunctionArray = AllFunctions.Array();
                if (FunctionArray.RemoveAll(ValidFunctionPredicate))
                { AllFunctions = TSet(FunctionArray); }
            }
            AllFunctions.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
            AllFunctions.Shrink();

            AllFunctionsExec = AllFunctions;
            AutoCallFunction.RemoveAll(PinNotFoundPredicate);
            AutoCallFunction.Shrink();
        }
        else
        {
            ResetPinByNames(AllFunctionsExec);
            ResetPinByNames(ExecFunction);
            ResetPinByNames(AutoCallFunction);
        }

        if (AllFunctionsExec.Num())
        {
            const auto PureFunctionPredicate = [TargetClass](const FName Name)
            {
                if (Name != NAME_None)
                {
                    if (const auto* Property = TargetClass->FindFunctionByName(Name))
                    { return Property->HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Const); }
                }
                return false;
            };

            {
                auto ExecFunctionArray = AllFunctionsExec.Array();
                if (ExecFunctionArray.RemoveAll(PureFunctionPredicate))
                { AllFunctionsExec = TSet(ExecFunctionArray); }
            }

            ExecFunction.RemoveAll(PinNotFoundPredicate);
            ExecFunction.RemoveAll(PureFunctionPredicate);
            ExecFunction.Shrink();
        }
        else
        { ResetPinByNames(ExecFunction); }

        for (auto& FuncName : AutoCallFunction)
        { FuncName.SetAllExclude(AllFunctions, AutoCallFunction); }

        for (auto& FuncName : ExecFunction)
        { FuncName.SetAllExclude(AllFunctionsExec, ExecFunction); }

        for (auto& DelegateName : InDelegate)
        { DelegateName.SetAllExclude(AllDelegates, InDelegate); }

        for (auto& DelegateName : OutDelegate)
        { DelegateName.SetAllExclude(AllDelegates, OutDelegate); }

        for (auto& Param : SpawnParam)
        { Param.SetAllExclude(AllParam, SpawnParam); }
    }
}

void UBtf_ExtendConstructObject_K2Node::CreatePinsForClassFunction(UClass* TargetClass, const FName FunctionName, const bool RetValPins)
{
    if (const auto* TargetFunction = TargetClass->FindFunctionByName(FunctionName))
    {
        const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();

        auto* ExecPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, TargetFunction->GetFName());
        ExecPin->PinToolTip = TargetFunction->GetToolTipText().ToString();

        {
            auto FunctionNameStr = TargetFunction->HasMetaData(FBlueprintMetadata::MD_DisplayName)
                ? TargetFunction->GetMetaData(FBlueprintMetadata::MD_DisplayName)
                : FunctionName.ToString();

            if (FunctionNameStr.StartsWith(FNames_Helper::InPrefix))
            { FunctionNameStr.RemoveAt(0, FNames_Helper::InPrefix.Len()); }

            if (FunctionNameStr.StartsWith(FNames_Helper::InitPrefix))
            { FunctionNameStr.RemoveAt(0, FNames_Helper::InitPrefix.Len()); }

            ExecPin->PinFriendlyName = FText::FromString(FunctionNameStr);
        }

        auto AllPinsGood = true;
        for (TFieldIterator<FProperty> PropertyIt(TargetFunction); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
        {
            const auto* Param = *PropertyIt;
            const auto IsFunctionInput =
                NOT Param->HasAnyPropertyFlags(CPF_ReturnParm) && (NOT Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm));
            const auto Direction = IsFunctionInput ? EGPD_Input : EGPD_Output;

            if (Direction == EGPD_Output && NOT RetValPins)
            { continue; }

            auto PinParams = FCreatePinParams{};
            PinParams.bIsReference = Param->HasAnyPropertyFlags(CPF_ReferenceParm) && IsFunctionInput;

            const auto ParamFullName = FName(*FString::Printf(TEXT("%s_%s"), *FunctionName.ToString(), *Param->GetFName().ToString()));
            auto* Pin = CreatePin(Direction, NAME_None, ParamFullName, PinParams);
            const auto PinGood = (Pin && K2Schema->ConvertPropertyToPinType(Param, Pin->PinType));

            if (PinGood)
            {
                Pin->bDefaultValueIsIgnored = Param->HasAllPropertyFlags(CPF_ReferenceParm) &&
                    (NOT TargetFunction->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm) || Pin->PinType.IsContainer());

                if (auto ParamValue = FString{};
                    K2Schema->FindFunctionParameterDefaultValue(TargetFunction, Param, ParamValue))
                { K2Schema->SetPinAutogeneratedDefaultValue(Pin, ParamValue); }
                else
                { K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin); }

                const auto& PinDisplayName = Param->HasMetaData(FBlueprintMetadata::MD_DisplayName)
                    ? Param->GetMetaData(FBlueprintMetadata::MD_DisplayName)
                    : Param->GetFName().ToString();

                if (PinDisplayName == FString(TEXT("ReturnValue")))
                {
                    const auto ReturnPropertyName = FString::Printf(TEXT("%s\n%s"), *FunctionName.ToString(), *PinDisplayName);
                    Pin->PinFriendlyName = FText::FromString(ReturnPropertyName);
                }
                else if (NOT PinDisplayName.IsEmpty())
                { Pin->PinFriendlyName = FText::FromString(PinDisplayName); }
                else if (TargetFunction->GetReturnProperty() == Param && TargetFunction->HasMetaData(FBlueprintMetadata::MD_ReturnDisplayName))
                {
                    const auto ReturnPropertyName = FString::Printf(
                        TEXT("%s_%s"),
                        *FunctionName.ToString(),
                        *(TargetFunction->GetMetaDataText(FBlueprintMetadata::MD_ReturnDisplayName).ToString()));
                    Pin->PinFriendlyName = FText::FromString(ReturnPropertyName);
                }
                Pin->PinToolTip = ParamFullName.ToString();
            }

            AllPinsGood = AllPinsGood && PinGood;
        }
        check(AllPinsGood);
    }
}

void UBtf_ExtendConstructObject_K2Node::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    if (NOT IsValid(ProxyClass))
    {
        CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: ProxyClass nullptr error. @@"), this);
        return;
    }

    const auto* Schema = CompilerContext.GetSchema();
    check(SourceGraph && Schema);

    // Create proxy object
    UK2Node_CallFunction* CreateProxyObjectNode = nullptr;
    UEdGraphPin* ProxyObjectPin = nullptr;
    if (NOT CreateProxyObject(CompilerContext, SourceGraph, CreateProxyObjectNode, ProxyObjectPin))
    { return; }

    // Cast proxy object if needed
    auto* CastOutput = CastProxyObjectIfNeeded(CompilerContext, SourceGraph, ProxyObjectPin);

    // Connect output object pin
    if (auto* const OutputObjectPin = FindPinChecked(OutPutObjectPinName, EGPD_Output);
        NOT CompilerContext.MovePinLinksToIntermediate(*OutputObjectPin, *CastOutput).CanSafeConnect())
    {
        CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: OutObjectPin Error. @@"), this);
    }

    // Create temporary variables for outputs
    const auto VariableOutputs = CreateVariableOutputs(CompilerContext);

    // Setup execution flow
    auto* OriginalThenPin = FindPinChecked(UEdGraphSchema_K2::PN_Then);
    auto* LastThenPin = CreateProxyObjectNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

    // Validate proxy object
    if (NOT ValidateProxyObject(CompilerContext, SourceGraph, ProxyObjectPin, LastThenPin))
    { return; }

    // Process delegates
    if (NOT ProcessInputDelegates(CompilerContext, SourceGraph, CastOutput, LastThenPin))
    { return; }

    if (NOT ProcessOutputDelegates(CompilerContext, SourceGraph, CastOutput, LastThenPin, VariableOutputs))
    { return; }

    // Verify we have delegates
    if (CreateProxyObjectNode->FindPinChecked(UEdGraphSchema_K2::PN_Then) == LastThenPin)
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("MissingDelegateProperties", "ExtendConstructObject: Proxy has no delegates defined. @@").ToString(), this);
        return;
    }

    // Connect spawn properties
    if (NOT ConnectSpawnProperties(ProxyClass, Schema, CompilerContext, SourceGraph, LastThenPin, ProxyObjectPin))
    {
        CompilerContext.MessageLog.Error(TEXT("ConnectSpawnProperties error. @@"), this);
    }

    // Process custom pins
    if (NOT CustomPins.IsEmpty())
    {
        if (NOT ProcessCustomPins(CompilerContext, SourceGraph, CastOutput, LastThenPin))
        { return; }
    }

    // Process auto-call functions
    if (NOT ProcessAutoCallFunctions(CompilerContext, SourceGraph, CastOutput, LastThenPin, VariableOutputs))
    { return; }

    // Process exec functions
    if (NOT ProcessExecFunctions(CompilerContext, SourceGraph, CastOutput))
    { return; }

    // Connect final execution pin
    if (NOT CompilerContext.CopyPinLinksToIntermediate(*OriginalThenPin, *LastThenPin).CanSafeConnect())
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "ExtendConstructObject: Internal connection error. @@").ToString(), this);
    }

    SpawnParam.Shrink();
    BreakAllNodeLinks();
}

bool UBtf_ExtendConstructObject_K2Node::CreateProxyObject(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph,
                                                         UK2Node_CallFunction*& OutProxyNode, UEdGraphPin*& OutProxyPin)
{
    const auto* Schema = CompilerContext.GetSchema();

    OutProxyNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    OutProxyNode->FunctionReference.SetExternalMember(ProxyFactoryFunctionName, ProxyFactoryClass);
    OutProxyNode->AllocateDefaultPins();

    if (NOT IsValid(OutProxyNode->GetTargetFunction()))
    {
        const auto ClassName = IsValid(ProxyFactoryClass) ? FText::FromString(ProxyFactoryClass->GetName()) : LOCTEXT("MissingClassString", "Unknown Class");
        const auto FormattedMessage = FText::Format(
                LOCTEXT("AsyncTaskErrorFmt", "BaseAsyncTask: Missing function {0} from class {1} for async task @@"),
                FText::FromString(ProxyFactoryFunctionName.GetPlainNameString()),
                ClassName)
                .ToString();

        CompilerContext.MessageLog.Error(*FormattedMessage, this);
        return false;
    }

    // Connect execution pin
    if (NOT CompilerContext.MovePinLinksToIntermediate(
            *FindPinChecked(UEdGraphSchema_K2::PN_Execute),
            *OutProxyNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute))
            .CanSafeConnect())
    {
        CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Failed to connect execution pins. @@"), this);
        return false;
    }

    // Handle self context
    if (SelfContext)
    {
        auto* const SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(this, SourceGraph);
        SelfNode->AllocateDefaultPins();
        auto* SelfPin = SelfNode->FindPinChecked(UEdGraphSchema_K2::PN_Self);

        if (NOT Schema->TryCreateConnection(SelfPin, OutProxyNode->FindPinChecked(WorldContextPinName)))
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Pin Self Context Error. @@"), this);
            return false;
        }
    }

    // Copy input pins
    for (auto* CurrentPin : Pins)
    {
        if (CurrentPin->PinName != UEdGraphSchema_K2::PSC_Self && FNodeHelper::ValidDataPin(CurrentPin, EGPD_Input))
        {
            if (auto* DestPin = OutProxyNode->FindPin(CurrentPin->PinName))
            {
                if (NOT CompilerContext.CopyPinLinksToIntermediate(*CurrentPin, *DestPin).CanSafeConnect())
                {
                    CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Failed to copy input pin links. @@"), this);
                    return false;
                }
            }
        }
    }

    // Set node guid
    OutProxyNode->FindPin(NodeGuidStrName)->DefaultValue = NodeGuid.ToString();

    OutProxyPin = OutProxyNode->GetReturnValuePin();
    return true;
}

bool UBtf_ExtendConstructObject_K2Node::ValidateProxyObject(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph,
                                                           UEdGraphPin* ProxyObjectPin, UEdGraphPin*& LastThenPin)
{
    const auto* Schema = CompilerContext.GetSchema();
    const auto IsValidFuncName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid);

    auto* ValidateProxyNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
    ValidateProxyNode->AllocateDefaultPins();

    auto* IsValidFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    IsValidFuncNode->FunctionReference.SetExternalMember(IsValidFuncName, UKismetSystemLibrary::StaticClass());
    IsValidFuncNode->AllocateDefaultPins();

    auto* IsValidInputPin = IsValidFuncNode->FindPinChecked(TEXT("Object"));
    auto* OriginalThenPin = FindPinChecked(UEdGraphSchema_K2::PN_Then);

    auto Success = Schema->TryCreateConnection(ProxyObjectPin, IsValidInputPin);
    Success &= Schema->TryCreateConnection(IsValidFuncNode->GetReturnValuePin(), ValidateProxyNode->GetConditionPin());
    Success &= Schema->TryCreateConnection(LastThenPin, ValidateProxyNode->GetExecPin());
    Success &= CompilerContext.CopyPinLinksToIntermediate(*OriginalThenPin, *ValidateProxyNode->GetElsePin()).CanSafeConnect();

    LastThenPin = ValidateProxyNode->GetThenPin();
    return Success;
}

UEdGraphPin* UBtf_ExtendConstructObject_K2Node::CastProxyObjectIfNeeded(FKismetCompilerContext& CompilerContext,
                                                                       UEdGraph* SourceGraph,
                                                                       UEdGraphPin* ProxyObjectPin)
{
    if (ProxyObjectPin->PinType.PinSubCategoryObject.Get() == ProxyClass)
    { return ProxyObjectPin; }

    const auto* Schema = CompilerContext.GetSchema();
    auto* CastNode = CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, SourceGraph);
    CastNode->SetPurity(true);
    CastNode->TargetType = ProxyClass;
    CastNode->AllocateDefaultPins();

    Schema->TryCreateConnection(ProxyObjectPin, CastNode->GetCastSourcePin());

    auto* CastOutput = CastNode->GetCastResultPin();
    check(CastOutput);
    return CastOutput;
}

TArray<UBtf_ExtendConstructObject_K2Node::FNodeHelper::FOutputPinAndLocalVariable>
UBtf_ExtendConstructObject_K2Node::CreateVariableOutputs(FKismetCompilerContext& CompilerContext)
{
    // Collect all custom payload pin names for quick lookup
    auto CustomPayloadPinNames = TSet<FName>{};
    for (const auto& CustomPin : CustomPins)
    {
        if (NOT CustomPin.PayloadType)
        { continue; }

        for (TFieldIterator<FProperty> PropertyIt(CustomPin.PayloadType); PropertyIt; ++PropertyIt)
        {
            const auto* Property = *PropertyIt;
            if (NOT Property->HasAnyPropertyFlags(CPF_Parm) && Property->HasAllPropertyFlags(CPF_BlueprintVisible))
            {
                CustomPayloadPinNames.Add(Property->GetFName());
            }
        }
    }

    // Lambda to check if a pin should have a variable output
    const auto ShouldCreateVariableForPin = [&](const UEdGraphPin* Pin) -> bool
    {
        const auto* OutputObjectPin = FindPinChecked(OutPutObjectPinName, EGPD_Output);
        return Pin != OutputObjectPin &&
               FNodeHelper::ValidDataPin(Pin, EGPD_Output) &&
               NOT CustomPayloadPinNames.Contains(Pin->PinName);
    };

    // Create temporary variables for eligible output pins
    auto VariableOutputs = TArray<FNodeHelper::FOutputPinAndLocalVariable>{};
    for (auto* CurrentPin : Pins)
    {
        if (NOT ShouldCreateVariableForPin(CurrentPin))
        { continue; }

        const auto& PinType = CurrentPin->PinType;
        auto* TempVarOutput = CompilerContext.SpawnInternalVariable(
            this,
            PinType.PinCategory,
            PinType.PinSubCategory,
            PinType.PinSubCategoryObject.Get(),
            PinType.ContainerType,
            PinType.PinValueType);

        if (NOT TempVarOutput->GetVariablePin())
        { continue; }

        CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *TempVarOutput->GetVariablePin());
        VariableOutputs.Add(FNodeHelper::FOutputPinAndLocalVariable(CurrentPin, TempVarOutput));
    }

    return VariableOutputs;
}

bool UBtf_ExtendConstructObject_K2Node::ProcessInputDelegates(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph,
                                                             UEdGraphPin* ProxyObjectPin, UEdGraphPin*& LastThenPin)
{
    const auto* Schema = CompilerContext.GetSchema();
    auto Success = true;

    for (const auto DelegateName : InDelegate)
    {
        if (DelegateName == NAME_None)
        { continue; }

        const auto* Property = ProxyClass->FindPropertyByName(DelegateName);
        if (NOT Property)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
            return false;
        }

        const auto* DelegateProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);
        if (NOT DelegateProperty || DelegateProperty->GetFName() != DelegateName)
        { continue; }

        auto* InDelegatePin = FindPinChecked(DelegateName, EGPD_Input);
        if (InDelegatePin->LinkedTo.Num() == 0)
        { continue; }

        auto* AssignDelegate = CompilerContext.SpawnIntermediateNode<UK2Node_AssignDelegate>(this, SourceGraph);
        AssignDelegate->SetFromProperty(DelegateProperty, false, DelegateProperty->GetOwnerClass());
        AssignDelegate->AllocateDefaultPins();

        Success &= Schema->TryCreateConnection(AssignDelegate->FindPinChecked(UEdGraphSchema_K2::PN_Self), ProxyObjectPin);
        Success &= Schema->TryCreateConnection(LastThenPin, AssignDelegate->FindPinChecked(UEdGraphSchema_K2::PN_Execute));

        auto* DelegatePin = AssignDelegate->GetDelegatePin();
        Success &= CompilerContext.MovePinLinksToIntermediate(*InDelegatePin, *DelegatePin).CanSafeConnect();

        if (NOT Success)
        {
            CompilerContext.MessageLog.Error(*LOCTEXT("Invalid_DelegateProperties", "Error @@").ToString(), this);
            return false;
        }

        LastThenPin = AssignDelegate->FindPinChecked(UEdGraphSchema_K2::PN_Then);
    }

    return Success;
}

bool UBtf_ExtendConstructObject_K2Node::ProcessOutputDelegates(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph,
                                                              UEdGraphPin* ProxyObjectPin, UEdGraphPin*& LastThenPin,
                                                              const TArray<FNodeHelper::FOutputPinAndLocalVariable>& VariableOutputs)
{
    auto Success = true;

    for (const auto DelegateName : OutDelegate)
    {
        if (DelegateName == NAME_None)
        { continue; }

        const auto* Property = ProxyClass->FindPropertyByName(DelegateName);
        if (NOT Property)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
            return false;
        }

        const auto* DelegateProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);
        if (NOT DelegateProperty || DelegateProperty->GetFName() != DelegateName)
        { continue; }

        if (NOT FindPin(DelegateProperty->GetFName()))
        {
            Success = false;
            CompilerContext.MessageLog.Error(*LOCTEXT("Invalid_DelegateProperties", "@@").ToString(), this);
            continue;
        }

        Success &= FNodeHelper::HandleDelegateImplementation(
            const_cast<FMulticastDelegateProperty*>(DelegateProperty),
            VariableOutputs,
            ProxyObjectPin,
            LastThenPin,
            this,
            SourceGraph,
            CompilerContext);

        if (NOT Success)
        {
            CompilerContext.MessageLog.Error(*LOCTEXT("Invalid_DelegateProperties", "@@").ToString(), this);
            return false;
        }
    }

    return Success;
}

bool UBtf_ExtendConstructObject_K2Node::ProcessCustomPins(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph,
                                                         UEdGraphPin* ProxyObjectPin, UEdGraphPin*& LastThenPin)
{
    auto* DelegateProperty = ProxyClass->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UBtf_TaskForge, OnCustomPinTriggered));
    auto* MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(DelegateProperty);

    if (NOT MulticastDelegateProperty)
    {
        CompilerContext.MessageLog.Error(*FString::Printf(TEXT("Failed to find delegate property for %s."), *GetNodeTitle(ENodeTitleType::FullTitle).ToString()));
        this->BreakAllNodeLinks();
        return false;
    }

    const auto Success = FNodeHelper::HandleCustomPinsImplementation(
        MulticastDelegateProperty,
        ProxyObjectPin,
        LastThenPin,
        this,
        SourceGraph,
        CustomPins,
        CompilerContext
    );

    if (NOT Success)
    {
        CompilerContext.MessageLog.Error(*FString::Printf(TEXT("Failed to handle delegate for %s."), *GetNodeTitle(ENodeTitleType::FullTitle).ToString()));
        return false;
    }

    return true;
}

bool UBtf_ExtendConstructObject_K2Node::ProcessAutoCallFunctions(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph,
                                                                UEdGraphPin* ProxyObjectPin, UEdGraphPin*& LastThenPin,
                                                                const TArray<FNodeHelper::FOutputPinAndLocalVariable>& VariableOutputs)
{
    const auto* Schema = CompilerContext.GetSchema();
    auto Success = true;

    for (const auto FunctionName : AutoCallFunction)
    {
        if (FunctionName == NAME_None)
        { continue; }

        const auto* TargetFunction = ProxyClass->FindFunctionByName(FunctionName);
        if (NOT IsValid(TargetFunction))
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
            continue;
        }

        auto* CallFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
        CallFunctionNode->FunctionReference.SetExternalMember(FunctionName, ProxyClass);
        CallFunctionNode->AllocateDefaultPins();

        auto* ActivateCallSelfPin = Schema->FindSelfPin(*CallFunctionNode, EGPD_Input);
        check(ActivateCallSelfPin);

        Success &= Schema->TryCreateConnection(ProxyObjectPin, ActivateCallSelfPin);
        if (NOT Success)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Failed to connect self pin. @@"), this);
            return false;
        }

        if (const auto IsPureFunc = CallFunctionNode->IsNodePure();
            NOT IsPureFunc)
        {
            auto* ActivateExecPin = CallFunctionNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute);
            auto* ActivateThenPin = CallFunctionNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

            Success &= Schema->TryCreateConnection(LastThenPin, ActivateExecPin);
            if (NOT Success)
            {
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Failed to connect execution pins. @@"), this);
                return false;
            }

            LastThenPin = ActivateThenPin;
        }

        // Connect function parameters
        const auto* EventSignature = CallFunctionNode->GetTargetFunction();
        check(EventSignature);

        for (TFieldIterator<FProperty> PropertyIt(EventSignature); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
        {
            const auto* Param = *PropertyIt;
            const auto IsFunctionInput = NOT Param->HasAnyPropertyFlags(CPF_ReturnParm) &&
                (NOT Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm));

            const auto Direction = IsFunctionInput ? EGPD_Input : EGPD_Output;

            auto* NodePin = FindParamPin(FunctionName.Name.ToString(), Param->GetFName(), Direction);
            if (NOT NodePin)
            {
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
                return false;
            }

            auto* FunctionPin = CallFunctionNode->FindPinChecked(Param->GetFName(), Direction);

            if (IsFunctionInput)
            {
                Success &= CompilerContext.MovePinLinksToIntermediate(*NodePin, *FunctionPin).CanSafeConnect();
            }
            else
            {
                const auto* OutputPair = VariableOutputs.FindByKey(NodePin);
                check(OutputPair);

                auto* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
                AssignNode->AllocateDefaultPins();

                Success &= Schema->TryCreateConnection(LastThenPin, AssignNode->GetExecPin());
                Success &= Schema->TryCreateConnection(OutputPair->TempVar->GetVariablePin(), AssignNode->GetVariablePin());
                AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());

                Success &= Schema->TryCreateConnection(AssignNode->GetValuePin(), FunctionPin);
                AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

                LastThenPin = AssignNode->GetThenPin();
            }
        }

        if (NOT Success)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Failed to connect function parameters. @@"), this);
            return false;
        }
    }

    return Success;
}

bool UBtf_ExtendConstructObject_K2Node::ProcessExecFunctions(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph,
                                                            UEdGraphPin* ProxyObjectPin)
{
    const auto* Schema = CompilerContext.GetSchema();
    const auto IsValidFuncName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid);
    auto Success = true;

    for (const auto FunctionName : ExecFunction)
    {
        if (FunctionName == NAME_None)
        { continue; }

        if (const auto* TargetFunction = ProxyClass->FindFunctionByName(FunctionName);
            NOT TargetFunction)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
            continue;
        }

        auto* ExecPin = FindPinChecked(FunctionName, EGPD_Input);

        // Create validation check
        auto* IsValidFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
        IsValidFuncNode->FunctionReference.SetExternalMember(IsValidFuncName, UKismetSystemLibrary::StaticClass());
        IsValidFuncNode->AllocateDefaultPins();

        auto* IfThenElse = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
        IfThenElse->AllocateDefaultPins();

        auto* IsValidInputPin = IsValidFuncNode->FindPinChecked(TEXT("Object"));
        Success &= Schema->TryCreateConnection(ProxyObjectPin, IsValidInputPin);
        Success &= Schema->TryCreateConnection(IsValidFuncNode->GetReturnValuePin(), IfThenElse->GetConditionPin());
        Success &= CompilerContext.MovePinLinksToIntermediate(*ExecPin, *IfThenElse->GetExecPin()).CanSafeConnect();

        // Create function call
        auto* CallFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
        CallFunctionNode->FunctionReference.SetExternalMember(FunctionName, ProxyClass);
        CallFunctionNode->AllocateDefaultPins();

        if (CallFunctionNode->IsNodePure())
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Exec function cannot be pure. @@"), this);
            continue;
        }

        auto* CallSelfPin = Schema->FindSelfPin(*CallFunctionNode, EGPD_Input);
        check(CallSelfPin);

        Success &= Schema->TryCreateConnection(ProxyObjectPin, CallSelfPin);
        Success &= Schema->TryCreateConnection(IfThenElse->GetThenPin(), CallFunctionNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));

        // Connect parameters
        const auto* EventSignature = CallFunctionNode->GetTargetFunction();
        for (TFieldIterator<FProperty> PropertyIt(EventSignature); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
        {
            const auto* Param = *PropertyIt;
            const auto IsFunctionInput = NOT Param->HasAnyPropertyFlags(CPF_ReturnParm) &&
                (NOT Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm));

            if (NOT IsFunctionInput)
            { continue; }

            auto* NodePin = FindParamPin(FunctionName.Name.ToString(), Param->GetFName(), EGPD_Input);
            if (NOT NodePin)
            {
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
                return false;
            }

            auto* FunctionPin = CallFunctionNode->FindPinChecked(Param->GetFName(), EGPD_Input);
            Success &= CompilerContext.MovePinLinksToIntermediate(*NodePin, *FunctionPin).CanSafeConnect();
        }
    }

    if (NOT Success)
    {
        CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Failed to process exec functions. @@"), this);
        return false;
    }

    return Success;
}

bool UBtf_ExtendConstructObject_K2Node::ConnectSpawnProperties(
    const UClass* ClassToSpawn,
    const UEdGraphSchema_K2* Schema,
    FKismetCompilerContext& CompilerContext,
    UEdGraph* SourceGraph,
    UEdGraphPin*& LastThenPin,
    UEdGraphPin* SpawnedActorReturnPin)
{
    auto IsErrorFree = true;
    for (const auto OldPinParamName : SpawnParam)
    {
        auto* SpawnVarPin = FindPin(OldPinParamName);

        if (NOT SpawnVarPin)
        { continue; }

        const auto HasDefaultValue = NOT SpawnVarPin->DefaultValue.IsEmpty() || NOT SpawnVarPin->DefaultTextValue.IsEmpty() || IsValid(SpawnVarPin->DefaultObject);
        if (SpawnVarPin->LinkedTo.Num() > 0 || HasDefaultValue)
        {
            if (SpawnVarPin->LinkedTo.Num() == 0)
            {
                const auto* Property = FindFProperty<FProperty>(ClassToSpawn, SpawnVarPin->PinName);
                if (NOT Property)
                { continue; }

                if (IsValid(ClassToSpawn->GetDefaultObject()))
                {
                    auto DefaultValueAsString = FString{};
                    FBlueprintEditorUtils::PropertyValueToString(
                        Property,
#if ENGINE_MINOR_VERSION < 3
                        reinterpret_cast<uint8*>(ClassToSpawn->ClassDefaultObject),
#else
                        reinterpret_cast<uint8*>(ClassToSpawn->GetDefaultObject()),
#endif
                        DefaultValueAsString,
                        this);

                    if (DefaultValueAsString == SpawnVarPin->DefaultValue)
                    { continue; }
                }
            }

            if (const auto* SetByNameFunction = Schema->FindSetVariableByNameFunction(SpawnVarPin->PinType))
            {
                auto* SetVarNode = SpawnVarPin->PinType.IsArray()
                    ? CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph)
                    : CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

                SetVarNode->SetFromFunction(SetByNameFunction);
                SetVarNode->AllocateDefaultPins();

                IsErrorFree &= Schema->TryCreateConnection(LastThenPin, SetVarNode->GetExecPin());
                if (NOT IsErrorFree)
                {
                    CompilerContext.MessageLog.Error(
                        *LOCTEXT("InternalConnectionError", "ExtendConstructObject: Internal connection error. @@").ToString(),
                        this);
                }
                LastThenPin = SetVarNode->GetThenPin();

                static const auto ObjectParamName = FName(TEXT("Object"));
                static const auto ValueParamName = FName(TEXT("Value"));
                static const auto PropertyNameParamName = FName(TEXT("PropertyName"));

                auto* ObjectPin = SetVarNode->FindPinChecked(ObjectParamName);
                SpawnedActorReturnPin->MakeLinkTo(ObjectPin);

                auto* PropertyNamePin = SetVarNode->FindPinChecked(PropertyNameParamName);
                PropertyNamePin->DefaultValue = SpawnVarPin->PinName.ToString();

                auto* ValuePin = SetVarNode->FindPinChecked(ValueParamName);
                if (SpawnVarPin->LinkedTo.Num() == 0 &&
                    SpawnVarPin->DefaultValue != FString() &&
                    SpawnVarPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte &&
                    SpawnVarPin->PinType.PinSubCategoryObject.IsValid() &&
                    SpawnVarPin->PinType.PinSubCategoryObject->IsA<UEnum>())
                {
                    auto* EnumLiteralNode = CompilerContext.SpawnIntermediateNode<UK2Node_EnumLiteral>(this, SourceGraph);
                    EnumLiteralNode->Enum = CastChecked<UEnum>(SpawnVarPin->PinType.PinSubCategoryObject.Get());
                    EnumLiteralNode->AllocateDefaultPins();
                    EnumLiteralNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(ValuePin);

                    auto* InputPin = EnumLiteralNode->FindPinChecked(UK2Node_EnumLiteral::GetEnumInputPinName());
                    InputPin->DefaultValue = SpawnVarPin->DefaultValue;
                }
                else
                {
                    if (NOT SpawnVarPin->PinType.IsArray() &&
                        SpawnVarPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct &&
                        SpawnVarPin->LinkedTo.Num() == 0)
                    {
                        ValuePin->PinType.PinCategory = SpawnVarPin->PinType.PinCategory;
                        ValuePin->PinType.PinSubCategory = SpawnVarPin->PinType.PinSubCategory;
                        ValuePin->PinType.PinSubCategoryObject = SpawnVarPin->PinType.PinSubCategoryObject;
                        CompilerContext.MovePinLinksToIntermediate(*SpawnVarPin, *ValuePin);
                    }
                    else
                    {
                        CompilerContext.MovePinLinksToIntermediate(*SpawnVarPin, *ValuePin);
                        SetVarNode->PinConnectionListChanged(ValuePin);
                    }
                }
            }
        }
    }
    return IsErrorFree;
}

bool UBtf_ExtendConstructObject_K2Node::FNodeHelper::ValidDataPin(const UEdGraphPin* Pin, EEdGraphPinDirection Direction)
{
    const auto ValidDataPin = Pin &&
        NOT Pin->bOrphanedPin &&
        (Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec);

    const auto ProperDirection = Pin && (Pin->Direction == Direction);

    return ValidDataPin && ProperDirection;
}

bool UBtf_ExtendConstructObject_K2Node::FNodeHelper::CreateDelegateForNewFunction(
    UEdGraphPin* DelegateInputPin,
    FName FunctionName,
    UK2Node* CurrentNode,
    UEdGraph* SourceGraph,
    FKismetCompilerContext& CompilerContext)
{
    const auto* Schema = CompilerContext.GetSchema();
    check(DelegateInputPin && Schema && CurrentNode && SourceGraph && (FunctionName != NAME_None));
    auto Result = true;

    auto* SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(CurrentNode, SourceGraph);
    SelfNode->AllocateDefaultPins();

    auto* CreateDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CreateDelegate>(CurrentNode, SourceGraph);
    CreateDelegateNode->AllocateDefaultPins();
    Result &= Schema->TryCreateConnection(DelegateInputPin, CreateDelegateNode->GetDelegateOutPin());
    Result &= Schema->TryCreateConnection(SelfNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), CreateDelegateNode->GetObjectInPin());
    CreateDelegateNode->SetFunction(FunctionName);

    return Result;
}

bool UBtf_ExtendConstructObject_K2Node::FNodeHelper::CopyEventSignature(UK2Node_CustomEvent* CENode, UFunction* Function, const UEdGraphSchema_K2* Schema)
{
    check(CENode && Function && Schema);

    auto Result = true;
    for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt && (PropertyIt->PropertyFlags & CPF_Parm); ++PropertyIt)
    {
        const auto* Param = *PropertyIt;
        if (NOT Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
        {
            auto PinType = FEdGraphPinType{};
            Result &= Schema->ConvertPropertyToPinType(Param, PinType);
            Result &= (nullptr != CENode->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output));
        }
    }
    return Result;
}

bool UBtf_ExtendConstructObject_K2Node::FNodeHelper::HandleDelegateImplementation(
    FMulticastDelegateProperty* CurrentProperty,
    const TArray<FNodeHelper::FOutputPinAndLocalVariable>& VariableOutputs,
    UEdGraphPin* ProxyObjectPin,
    UEdGraphPin*& InOutLastThenPin,
    UK2Node* CurrentNode,
    UEdGraph* SourceGraph,
    FKismetCompilerContext& CompilerContext)
{
    auto IsErrorFree = true;
    const auto* Schema = CompilerContext.GetSchema();
    check(CurrentProperty && ProxyObjectPin && InOutLastThenPin && CurrentNode && SourceGraph && Schema);

    auto* PinForCurrentDelegateProperty = CurrentNode->FindPin(CurrentProperty->GetFName());
    if (NOT PinForCurrentDelegateProperty || (UEdGraphSchema_K2::PC_Exec != PinForCurrentDelegateProperty->PinType.PinCategory))
    {
        const auto ErrorMessage = FText::Format(
            LOCTEXT("WrongDelegateProperty", "BaseAsyncTask: Cannot find execution pin for delegate "),
            FText::FromString(CurrentProperty->GetName()));
        CompilerContext.MessageLog.Error(*ErrorMessage.ToString(), CurrentNode);
        return false;
    }

    auto* CurrentCeNode = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(CurrentNode, SourceGraph);
    {
        auto* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(CurrentNode, SourceGraph);
        AddDelegateNode->SetFromProperty(CurrentProperty, false, CurrentProperty->GetOwnerClass());
        AddDelegateNode->AllocateDefaultPins();
        IsErrorFree &= Schema->TryCreateConnection(AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), ProxyObjectPin);
        IsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
        InOutLastThenPin = AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

        CurrentCeNode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *CurrentProperty->GetName(), *CompilerContext.GetGuid(CurrentNode));
        CurrentCeNode->AllocateDefaultPins();

        IsErrorFree &= FNodeHelper::CreateDelegateForNewFunction(
            AddDelegateNode->GetDelegatePin(),
            CurrentCeNode->GetFunctionName(),
            CurrentNode,
            SourceGraph,
            CompilerContext);
        IsErrorFree &= FNodeHelper::CopyEventSignature(CurrentCeNode, AddDelegateNode->GetDelegateSignature(), Schema);
    }

    auto* LastActivatedNodeThen = CurrentCeNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

    const auto DelegateNameStr = CurrentProperty->GetName();
    for (const auto& OutputPair : VariableOutputs)
    {
        auto PinNameStr = OutputPair.OutputPin->PinName.ToString();

        if (NOT PinNameStr.StartsWith(DelegateNameStr))
        { continue; }

        PinNameStr.RemoveAt(0, DelegateNameStr.Len() + 1);

        auto* PinWithData = CurrentCeNode->FindPin(FName(*PinNameStr));

        if (NOT PinWithData)
        { continue; }

        auto* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(CurrentNode, SourceGraph);
        AssignNode->AllocateDefaultPins();
        IsErrorFree &= Schema->TryCreateConnection(LastActivatedNodeThen, AssignNode->GetExecPin());
        IsErrorFree &= Schema->TryCreateConnection(OutputPair.TempVar->GetVariablePin(), AssignNode->GetVariablePin());
        AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());
        IsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), PinWithData);
        AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

        LastActivatedNodeThen = AssignNode->GetThenPin();
    }

    IsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*PinForCurrentDelegateProperty, *LastActivatedNodeThen).CanSafeConnect();
    return IsErrorFree;
}

bool UBtf_ExtendConstructObject_K2Node::FNodeHelper::HandleCustomPinsImplementation(
    FMulticastDelegateProperty* CurrentProperty,
    UEdGraphPin* ProxyObjectPin, UEdGraphPin*& InOutLastThenPin, UK2Node* CurrentNode, UEdGraph* SourceGraph,
    TArray<FCustomOutputPin> OutputNames, FKismetCompilerContext& CompilerContext)
{
    auto IsErrorFree = true;
    const auto* Schema = CompilerContext.GetSchema();
    check(CurrentProperty && ProxyObjectPin && InOutLastThenPin && CurrentNode && SourceGraph && Schema);

    {
        auto* CurrentCeNode   = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(CurrentNode, SourceGraph);
        auto* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(CurrentNode, SourceGraph);
        AddDelegateNode->SetFromProperty(CurrentProperty, false, CurrentProperty->GetOwnerClass());
        AddDelegateNode->AllocateDefaultPins();
        IsErrorFree &= Schema->TryCreateConnection(AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), ProxyObjectPin);
        IsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
        InOutLastThenPin = AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

        CurrentCeNode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *CurrentProperty->GetName(), *CompilerContext.GetGuid(CurrentNode));
        CurrentCeNode->AllocateDefaultPins();

        IsErrorFree &= FNodeHelper::CreateDelegateForNewFunction(
            AddDelegateNode->GetDelegatePin(),
            CurrentCeNode->GetFunctionName(),
            CurrentNode,
            SourceGraph,
            CompilerContext);
        IsErrorFree &= FNodeHelper::CopyEventSignature(CurrentCeNode, AddDelegateNode->GetDelegateSignature(), Schema);

        auto* PinNamePin = CurrentCeNode->FindPin(TEXT("PinName"));
        auto* DataPin = CurrentCeNode->FindPin(TEXT("Data"));

        auto* SwitchNode = CompilerContext.SpawnIntermediateNode<UK2Node_SwitchName>(CurrentNode, SourceGraph);
        auto ConvertedOutputNames = TArray<FName>{};
        for (const auto& CurrentPin : OutputNames)
        { ConvertedOutputNames.Add(FName(CurrentPin.PinName)); }

        SwitchNode->PinNames = ConvertedOutputNames;
        SwitchNode->AllocateDefaultPins();

        Schema->TryCreateConnection(CurrentCeNode->FindPinChecked(UEdGraphSchema_K2::PN_Then), SwitchNode->GetExecPin());
        Schema->TryCreateConnection(PinNamePin, SwitchNode->GetSelectionPin());

        for (int32 PinIndex = 0; PinIndex < OutputNames.Num(); ++PinIndex)
        {
            const auto& OutputName = FName(OutputNames[PinIndex].PinName);
            if (auto* SwitchCasePin = SwitchNode->FindPin(OutputName.ToString()))
            {
                if (OutputNames[PinIndex].PayloadType)
                {
                    auto* GetInstancedStructNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(CurrentNode, SourceGraph);
                    GetInstancedStructNode->SetFromFunction(
                        UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(
                            GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, GetInstancedStructValue)));
                    GetInstancedStructNode->AllocateDefaultPins();

                    Schema->TryCreateConnection(DataPin, GetInstancedStructNode->FindPin(TEXT("InstancedStruct")));
                    Schema->TryCreateConnection(SwitchCasePin, GetInstancedStructNode->GetExecPin());

                    auto* BreakStructNode = CompilerContext.SpawnIntermediateNode<UK2Node_BreakStruct>(CurrentNode, SourceGraph);
                    BreakStructNode->StructType = OutputNames[PinIndex].PayloadType;
                    BreakStructNode->AllocateDefaultPins();
                    BreakStructNode->bMadeAfterOverridePinRemoval = true;

                    auto* ValuePin = GetInstancedStructNode->FindPin(TEXT("Value"));
                    auto* StructInputPin = BreakStructNode->FindPin(OutputNames[PinIndex].PayloadType->GetFName());
                    Schema->TryCreateConnection(ValuePin, StructInputPin);

                    for (TFieldIterator<FProperty> PropertyIt(OutputNames[PinIndex].PayloadType); PropertyIt; ++PropertyIt)
                    {
                        const FProperty* Property = *PropertyIt;
                        if (NOT Property->HasAnyPropertyFlags(CPF_Parm) &&
                            Property->HasAllPropertyFlags(CPF_BlueprintVisible))
                        {
                            if (auto* BreakPin = BreakStructNode->FindPin(Property->GetFName()))
                            {
                                const FName PayloadPinName = Property->GetFName();

                                if (auto* NodeOutputPin = CurrentNode->FindPin(PayloadPinName))
                                { CompilerContext.MovePinLinksToIntermediate(*NodeOutputPin, *BreakPin); }
                            }
                        }
                    }

                    if (auto* NodeOutputPin = CurrentNode->FindPin(OutputName))
                    {
                        auto* ValidPin = GetInstancedStructNode->FindPin(TEXT("Valid"));
                        CompilerContext.MovePinLinksToIntermediate(*NodeOutputPin, *ValidPin);
                    }
                }
                else
                {
                    if (auto* NodeOutputPin = CurrentNode->FindPin(OutputName))
                    { CompilerContext.MovePinLinksToIntermediate(*NodeOutputPin, *SwitchCasePin); }
                }
            }
        }
    }

    return IsErrorFree;
}

void UBtf_ExtendConstructObject_K2Node::CollectSpawnParam(UClass* TargetClass, const bool FullRefresh)
{
    const auto InPrefix = FString::Printf(TEXT("In_"));
    const auto OutPrefix = FString::Printf(TEXT("Out_"));
    const auto InitPrefix = FString::Printf(TEXT("Init_"));

    {
        for (TFieldIterator<FProperty> PropertyIt(TargetClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
        {
            const auto* Property = *PropertyIt;
            const auto IsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
            const auto IsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
            const auto IsSettableExternally = NOT Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
            const auto PropertyName = Property->GetFName();

            if (NOT Property->HasAnyPropertyFlags(CPF_Parm) && NOT IsDelegate)
            {
                if (IsExposedToSpawn && IsSettableExternally && Property->HasAllPropertyFlags(CPF_BlueprintVisible))
                {
                    AllParam.Add(PropertyName);
                    SpawnParam.AddUnique(FBtf_NameSelect{PropertyName});
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
                    AllParam.Add(PropertyName);
                }
            }
        }
    }

    {
        for (TFieldIterator<UField> FieldIt(TargetClass); FieldIt; ++FieldIt)
        {
            if (const auto* TargetFunction = Cast<UFunction>(*FieldIt))
            {
                if (TargetFunction->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public) &&
                    NOT TargetFunction->HasAnyFunctionFlags(
                        FUNC_Static | FUNC_UbergraphFunction | FUNC_Delegate | FUNC_Private | FUNC_Protected | FUNC_EditorOnly) &&
                    NOT TargetFunction->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) &&
                    NOT TargetFunction->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction) &&
                    NOT FObjectEditorUtils::IsFunctionHiddenFromClass(TargetFunction, TargetClass))
                {
                    const auto FunctionName = TargetFunction->GetFName();

                    const auto OldNum = AllFunctions.Num();
                    AllFunctions.Add(FunctionName);

                    if (TargetFunction->GetBoolMetaData(FName("ExposeAutoCall")) || FunctionName.ToString().StartsWith(InitPrefix))
                    { AutoCallFunction.AddUnique(FBtf_NameSelect{FunctionName}); }

                    if (const auto NewFunction = OldNum - AllFunctions.Num() != 0)
                    {
                        if (FunctionName.ToString().StartsWith(InPrefix) && NOT TargetFunction->HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Const))
                        {
                            if (FullRefresh)
                            { ExecFunction.AddUnique(FBtf_NameSelect{FunctionName}); }
                        }
                    }
                }
            }
        }

        if (FullRefresh)
        {
            Algo::Reverse(ExecFunction);
            Algo::Reverse(AutoCallFunction);
        }
    }

    {
        const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
        const auto GraphType = K2Schema->GetGraphType(GetGraph());
        const auto AllowOutDelegate = GraphType != EGraphType::GT_Function;

        for (TFieldIterator<FProperty> PropertyIt(TargetClass); PropertyIt; ++PropertyIt)
        {
            if (const auto* DelegateProperty = CastField<FMulticastDelegateProperty>(*PropertyIt))
            {
                if (DelegateProperty->HasAnyPropertyFlags(FUNC_Private | CPF_Protected | FUNC_EditorOnly) ||
                    NOT DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintAssignable) ||
                    DelegateProperty->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) ||
                    DelegateProperty->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction))
                { continue; }

                if (const auto TargetFunction = DelegateProperty->SignatureFunction)
                {
                    if (NOT TargetFunction->HasAllFunctionFlags(FUNC_Public) ||
                        TargetFunction->HasAnyFunctionFlags(
                            FUNC_Static | FUNC_BlueprintPure | FUNC_Const | FUNC_UbergraphFunction | FUNC_Private | FUNC_Protected | FUNC_EditorOnly) ||
                        TargetFunction->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) ||
                        TargetFunction->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction))
                    { continue; }
                }

                const auto DelegateName = DelegateProperty->GetFName();
                AllDelegates.Add(DelegateName);

                if (NOT FullRefresh)
                {
                    if (OutDelegate.Contains(DelegateName))
                    {
                        if (NOT AllowOutDelegate)
                        {
                            OutDelegate.Remove(FBtf_NameSelect{DelegateName});
                            InDelegate.AddUnique(FBtf_NameSelect{DelegateName});
                        }
                    }
                }
                else
                {
                    if (DelegateName.ToString().StartsWith(InPrefix) ||
                        (DelegateName.ToString().StartsWith(OutPrefix) && NOT AllowOutDelegate))
                    { InDelegate.AddUnique(FBtf_NameSelect{DelegateName}); }
                    else if (DelegateName.ToString().StartsWith(OutPrefix) && AllowOutDelegate)
                    { OutDelegate.AddUnique(FBtf_NameSelect{DelegateName}); }
                    else
                    { continue; }
                }

                if (DelegateName != NAME_None)
                {
                    auto AllPinsGood = true;
                    auto DelegateNameStr = DelegateName.ToString();

                    if (DelegateNameStr.StartsWith(OutPrefix))
                    {
                        DelegateNameStr.RemoveAt(0, OutPrefix.Len());
                        if (const auto DelegateSignatureFunction = DelegateProperty->SignatureFunction)
                        {
                            for (TFieldIterator<FProperty> DelegatePropertyIt(DelegateSignatureFunction); DelegatePropertyIt && (DelegatePropertyIt->PropertyFlags & CPF_Parm); ++DelegatePropertyIt)
                            {
                                const auto ParamName = FName(*FString::Printf(TEXT("%s_%s"), *DelegateNameStr, *(*DelegatePropertyIt)->GetFName().ToString()));
                                AllPinsGood &= FindPin(ParamName, EGPD_Output) != nullptr;
                            }

                            for (const auto* Pin : Pins)
                            {
                                if (auto PinNameStr = Pin->GetName();
                                    PinNameStr.StartsWith(DelegateNameStr))
                                {
                                    PinNameStr.RemoveAt(0, DelegateNameStr.Len() + 1);
                                    AllPinsGood &= DelegateSignatureFunction->FindPropertyByName(FName(*PinNameStr)) != nullptr;
                                }
                            }
                        }
                    }

                    if (NOT AllPinsGood)
                    {
                        if (DelegateNameStr.StartsWith(InPrefix))
                        { DelegateNameStr.RemoveAt(0, InPrefix.Len()); }

                        if (DelegateNameStr.StartsWith(OutPrefix))
                        { DelegateNameStr.RemoveAt(0, OutPrefix.Len()); }

                        Pins.RemoveAll(
                            [&DelegateNameStr](UEdGraphPin* Pin)
                            {
                                if (Pin->GetName().StartsWith(DelegateNameStr))
                                {
                                    Pin->MarkAsGarbage();
                                    return true;
                                }
                                return false;
                            });

                        check(NOT FindPin(DelegateName));
                    }
                }
                continue;
            }

            if (const auto* DelegateProperty = CastField<FDelegateProperty>(*PropertyIt))
            {
                check(DelegateProperty);
                // TODO: Handle non-multicast delegate properties
            }
        }
    }

    Algo::Sort(
        AutoCallFunction,
        [&](const FName& A, const FName& B)
        {
            const auto AStr = A.ToString();
            const auto BStr = B.ToString();
            const auto AInit = AStr.StartsWith(InitPrefix);
            const auto BInit = BStr.StartsWith(InitPrefix);
            return (AInit && NOT BInit) ||
                ((AInit && BInit) ? UBtf_ExtendConstructObject_Utils::LessSuffix(A, AStr, B, BStr) : A.FastLess(B));
        });
}

#undef LOCTEXT_NAMESPACE
// --------------------------------------------------------------------------------------------------------------------
