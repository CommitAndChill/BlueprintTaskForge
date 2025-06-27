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
    {
        return FindPin(UEdGraphSchema_K2::PSC_Self);
    }
    return FindPin(WorldContextPinName);
}

#if WITH_EDITORONLY_DATA
void UBtf_ExtendConstructObject_K2Node::PostEditChangeProperty(FPropertyChangedEvent& e)
{
    auto NeedReconstruct = false;
    if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, AutoCallFunction))
    {
        RefreshNames(ExecFunction, false);
        for (auto& It : AutoCallFunction)
        {
            It.SetAllExclude(AllFunctions, AutoCallFunction);
            ExecFunction.Remove(It);
        }
        NeedReconstruct = true;
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, ExecFunction))
    {
        RefreshNames(ExecFunction, false);
        for (auto& It : ExecFunction)
        {
            It.SetAllExclude(AllFunctionsExec, ExecFunction);
            AutoCallFunction.Remove(It);
        }
        NeedReconstruct = true;
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, InDelegate))
    {
        RefreshNames(InDelegate, false);
        for (auto& It : InDelegate)
        {
            It.SetAllExclude(AllDelegates, InDelegate);
            OutDelegate.Remove(It);
        }
        NeedReconstruct = true;
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, OutDelegate))
    {
        RefreshNames(OutDelegate, false);
        for (auto& It : OutDelegate)
        {
            It.SetAllExclude(AllDelegates, OutDelegate);
            InDelegate.Remove(It);
        }
        NeedReconstruct = true;
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, SpawnParam))
    {
        RefreshNames(SpawnParam, false);
        for (auto& It : SpawnParam)
        {
            It.SetAllExclude(AllParam, SpawnParam);
        }
        NeedReconstruct = true;
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, OwnerContextPin))
    {
        NeedReconstruct = true;

        if (OwnerContextPin)
        {
            SelfContext = false;
        }
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, AllowInstance))
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
                    TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent PropertyChangedEvent)
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

                TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent PropertyChangedEvent)
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
        {
            DetailsPanelBuilder->ForceRefreshDetails();
        }
    }

    if (NeedReconstruct)
    {
        Modify();
        ReconstructNode();
    }

    auto IsDirty = false;
    auto PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
    if (PropertyName == TEXT("CustomPins"))
    {
        IsDirty = true;
    }

    if (IsDirty)
    {
        ReconstructNode();
    }

    Super::PostEditChangeProperty(e);
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
        {
            ProxyClass = nullptr;
        }
    }

    Super::Serialize(Ar);

    if (Ar.IsLoading())
    {
        if (IsValid(ProxyClass) && NOT ProxyClass->IsChildOf(UObject::StaticClass()))
        {
            ProxyClass = nullptr;
        }
        if (NOT OwnerContextPin && FindPin(WorldContextPinName))
        {
            SelfContext = false;
            OwnerContextPin = true;
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
    Super::GetNodeContextMenuActions(Menu, Context);
    if (Context->bIsDebugging) return;

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
                    [&](UToolMenu* InMenu)
                    {
                        auto& SubMenuSection = InMenu->AddSection("AddSpawnParamPin", FText::FromName(FName("AddSpawnParamPin")));
                        for (const auto& It : AllParam)
                        {
                            if (NOT SpawnParam.Contains(It))
                            {
                                SubMenuSection.AddMenuEntry(
                                    It,
                                    FText::FromName(It),
                                    FText(),
                                    FSlateIcon(),
                                    FUIAction(
                                        FExecuteAction::CreateUObject(
                                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                                            &UBtf_ExtendConstructObject_K2Node::AddSpawnParam,
                                            It),
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
                    [&](UToolMenu* InMenu)
                    {
                        auto& SubMenuSection = InMenu->AddSection("AddAutoCallFunction", LOCTEXT("AddAutoCallFunction", "AddAutoCallFunction"));
                        for (const auto& It : AllFunctions)
                        {
                            if (NOT AutoCallFunction.Contains(It))
                            {
                                SubMenuSection.AddMenuEntry(
                                    It,
                                    FText::FromName(It),
                                    FText(),
                                    FSlateIcon(),
                                    FUIAction(
                                        FExecuteAction::CreateUObject(
                                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                                            &UBtf_ExtendConstructObject_K2Node::AddAutoCallFunction,
                                            It),
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
                    [&](UToolMenu* InMenu)
                    {
                        auto& SubMenuSection = InMenu->AddSection("AddExecCallFunction", LOCTEXT("AddExecCallFunction", "AddCallFunctionPin"));
                        for (const auto& It : AllFunctionsExec)
                        {
                            if (NOT ExecFunction.Contains(It))
                            {
                                SubMenuSection.AddMenuEntry(
                                    It,
                                    FText::FromName(It),
                                    FText(),
                                    FSlateIcon(),
                                    FUIAction(
                                        FExecuteAction::CreateUObject(
                                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                                            &UBtf_ExtendConstructObject_K2Node::AddInputExec,
                                            It),
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
                    [&](UToolMenu* InMenu)
                    {
                        auto& SubMenuSection = InMenu->AddSection("AddInputDelegatePin", FText::FromName(FName("AddInputDelegatePin")));
                        for (const auto& It : AllDelegates)
                        {
                            if (NOT InDelegate.Contains(It))
                            {
                                SubMenuSection.AddMenuEntry(
                                    It,
                                    FText::FromName(It),
                                    FText(),
                                    FSlateIcon(),
                                    FUIAction(
                                        FExecuteAction::CreateUObject(
                                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                                            &UBtf_ExtendConstructObject_K2Node::AddInputDelegate,
                                            It),
                                        FIsActionChecked()));
                            }
                        }
                    }));

            Section.AddSubMenu(
                FName("Add_OutputDelegatePin"),
                FText::FromName(FName("AddOutputDelegatePin")),
                FText(),
                FNewToolMenuDelegate::CreateLambda(
                    [&](UToolMenu* InMenu)
                    {
                        auto& SubMenuSection = InMenu->AddSection("SubAsyncTaskSubMenu", FText::FromName(FName("AddOutputDelegatePin")));
                        for (const auto& It : AllDelegates)
                        {
                            if (NOT OutDelegate.Contains(It))
                            {
                                SubMenuSection.AddMenuEntry(
                                    It,
                                    FText::FromName(It),
                                    FText(),
                                    FSlateIcon(),
                                    FUIAction(
                                        FExecuteAction::CreateUObject(
                                            const_cast<UBtf_ExtendConstructObject_K2Node*>(this),
                                            &UBtf_ExtendConstructObject_K2Node::AddOutputDelegate,
                                            It),
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
        if (Context->Pin->PinName == UEdGraphSchema_K2::PN_Execute || Context->Pin->PinName == UEdGraphSchema_K2::PN_Then) return;

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
    const auto ActionKey = GetClass();
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        auto* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);

        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

FText UBtf_ExtendConstructObject_K2Node::GetMenuCategory() const
{
    if (const auto* TargetFunction = GetFactoryFunction())
    {
        return UK2Node_CallFunction::GetDefaultCategoryForFunction(TargetFunction, FText::GetEmpty());
    }
    return FText();
}

bool UBtf_ExtendConstructObject_K2Node::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
    const auto GraphType = TargetGraph->GetSchema()->GetGraphType(TargetGraph);
    const auto IsCompatible = GraphType == EGraphType::GT_Ubergraph || GraphType == EGraphType::GT_Macro || GraphType == EGraphType::GT_Function;
    return IsCompatible && Super::IsCompatibleWithGraph(TargetGraph);
}

void UBtf_ExtendConstructObject_K2Node::GetMenuEntries(struct FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    Super::GetMenuEntries(ContextMenuBuilder);
}

void UBtf_ExtendConstructObject_K2Node::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
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

    const auto* ClassPin = FindPin(ClassPinName, EGPD_Input);
    if (NOT (IsValid(ProxyClass) && ClassPin != nullptr && ClassPin->Direction == EGPD_Input))
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
        auto Errors = Task->ValidateNodeDuringCompilation();
        for (const auto& It : Errors)
        {
            MessageLog.Error(
                *FText::Format(
                     LOCTEXT("ExtendConstructObjectPlacedInGraph", "{0} @@"),
                     FText::FromString(It)).ToString(), this);
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
    if (NOT PinTooltipsValid)
    {
        for (auto* P : Pins)
        {
            if (P->Direction == EGPD_Input)
            {
                P->PinToolTip.Reset();
                GeneratePinTooltip(*P);
            }
        }
        PinTooltipsValid = true;
    }
    return Super::GetPinHoverText(Pin, HoverTextOut);
}

void UBtf_ExtendConstructObject_K2Node::EarlyValidation(class FCompilerResultsLog& MessageLog) const
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
    const auto* const K2Schema = Cast<const UEdGraphSchema_K2>(Schema);

    if (NOT IsValid(K2Schema))
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

bool UBtf_ExtendConstructObject_K2Node::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const
{
    const auto* SourceBlueprint = GetBlueprint();

    const auto ProxyFactoryResult = (IsValid(ProxyFactoryClass)) && (ProxyFactoryClass->ClassGeneratedBy != SourceBlueprint);
    if (ProxyFactoryResult && OptionalOutput != nullptr)
    {
        OptionalOutput->AddUnique(ProxyFactoryClass);
    }

    const auto ProxyResult = (IsValid(ProxyClass)) && (ProxyClass->ClassGeneratedBy != SourceBlueprint);
    if (ProxyResult && OptionalOutput != nullptr)
    {
        OptionalOutput->AddUnique(ProxyClass);
    }

    const auto SuperResult = Super::HasExternalDependencies(OptionalOutput);

    return SuperResult || ProxyFactoryResult || ProxyResult;
}

FText UBtf_ExtendConstructObject_K2Node::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (IsValid(ProxyClass))
    {
        auto Str = ProxyClass->GetName();
        Str.RemoveFromEnd(FNames_Helper::CompiledFromBlueprintSuffix, ESearchCase::Type::CaseSensitive);
        return FText::FromString(FString::Printf(TEXT("%s"), *Str));
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
    if (GetInstanceOrDefaultObject()->Get_NodeTitleColor(CustomColor))
    {
        return CustomColor;
    }

    return Super::GetNodeTitleColor();
}

FName UBtf_ExtendConstructObject_K2Node::GetCornerIcon() const
{
    if (IsValid(ProxyClass))
    {
        if (OutDelegate.Num() == 0)
        {
            return FName();
        }
    }
    return TEXT("Graph.Latent.LatentIcon");
}

bool UBtf_ExtendConstructObject_K2Node::UseWorldContext() const
{
    const auto* BP = GetBlueprint();
    const auto ParentClass = IsValid(BP) ? BP->ParentClass : nullptr;
    return IsValid(ParentClass) ? ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin) != nullptr : false;
}

FString UBtf_ExtendConstructObject_K2Node::GetPinMetaData(const FName InPinName, const FName InKey)
{
    if (const auto* Metadata = PinMetadataMap.Find(InPinName))
    {
        if (const auto* Value = Metadata->Find(InKey))
        {
            return *Value;
        }
    }

    return Super::GetPinMetaData(InPinName, InKey);
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
    for (auto& It : AutoCallFunction)
    {
        It.SetAllExclude(AllFunctions, AutoCallFunction);
        ExecFunction.Remove(It);
    }
    RefreshNames(ExecFunction);
    for (auto& It : ExecFunction)
    {
        It.SetAllExclude(AllFunctionsExec, ExecFunction);
    }
    RefreshNames(InDelegate);
    for (auto& It : InDelegate)
    {
        It.SetAllExclude(AllDelegates, InDelegate);
        OutDelegate.Remove(It);
    }
    RefreshNames(OutDelegate);
    for (auto& It : OutDelegate)
    {
        It.SetAllExclude(AllDelegates, OutDelegate);
    }

    AllowInstance = false;
    TaskInstance = nullptr;

    Super::PostPasteNode();
}

UObject* UBtf_ExtendConstructObject_K2Node::GetJumpTargetForDoubleClick() const
{
    if (IsValid(ProxyClass))
    {
        return ProxyClass->ClassGeneratedBy;
    }
    return nullptr;
}

bool UBtf_ExtendConstructObject_K2Node::CanBePlacedInGraph() const
{
    if (auto* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(GetGraph()))
    {
        if (IsValid(ProxyClass) && IsValid(Cast<UBtf_TaskForge>(ProxyClass->ClassDefaultObject)) && Cast<UBtf_TaskForge>(ProxyClass->ClassDefaultObject)->ClassLimitations.IsValidIndex(0))
        {
            for (auto& CurrentClass : Cast<UBtf_TaskForge>(ProxyClass->ClassDefaultObject)->ClassLimitations)
            {
                if (NOT CurrentClass.IsNull())
                {
                    if (CurrentClass->GetClassPathName().ToString() == Blueprint->GetPathName() + "_C")
                    {
                        return true;
                    }
                }
            }

            return false;
        }
    }

    return true;
}

UBtf_TaskForge* UBtf_ExtendConstructObject_K2Node::GetInstanceOrDefaultObject() const
{
    return AllowInstance && IsValid(TaskInstance) ? TaskInstance : ProxyClass->GetDefaultObject<UBtf_TaskForge>();
}

void UBtf_ExtendConstructObject_K2Node::GenerateCustomOutputPins()
{
    if (IsValid(ProxyClass))
    {
        if (auto* TargetClassAsBlueprintTask = GetInstanceOrDefaultObject())
        {
            auto OldCustomPins = CustomPins;
            CustomPins = TargetClassAsBlueprintTask->Get_CustomOutputPins();

            const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

            for (const auto& Pin : CustomPins)
            {
                FString PinNameStr = Pin.PinName;

                // Create the execution pin for this custom output
                if (!FindPin(PinNameStr))
                {
                    UEdGraphPin* ExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, FName(PinNameStr));
                    ExecPin->PinToolTip = Pin.Tooltip;
                }

                // If this pin has a payload type, create output pins for each property
                if (Pin.PayloadType)
                {
                    for (TFieldIterator<FProperty> It(Pin.PayloadType); It; ++It)
                    {
                        FProperty* Property = *It;

                        if (!Property->HasAnyPropertyFlags(CPF_Parm) &&
                            Property->HasAllPropertyFlags(CPF_BlueprintVisible))
                        {

                            // Create pin name as CustomPinName_PropertyName
                            FName PayloadPinName = Property->GetFName();

                            if (!FindPin(PayloadPinName))
                            {
                                FEdGraphPinType PinType;
                                if (K2Schema->ConvertPropertyToPinType(Property, PinType))
                                {
                                    UEdGraphPin* PayloadPin = CreatePin(EGPD_Output, PinType, PayloadPinName);
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
                    // Clean up old payload pins if they exist
                    for (auto& OldPin : OldCustomPins)
                    {
                        if (OldPin.PinName == Pin.PinName)
                        {
                            // Remove any associated payload pins
                            TArray<UEdGraphPin*> PinsToRemove;
                            for (UEdGraphPin* ExistingPin : Pins)
                            {
                                FString ExistingPinName = ExistingPin->PinName.ToString();
                                if (ExistingPinName.StartsWith(PinNameStr + TEXT("_")))
                                {
                                    PinsToRemove.Add(ExistingPin);
                                }
                            }

                            for (UEdGraphPin* PinToRemove : PinsToRemove)
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
        TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent PropertyChangedEvent)
        {
            ReconstructNode();
        });
    }

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
        {
            ProxyClass = Cast<UClass>(Pin->DefaultObject);
        }
        else if (Pin && (Pin->LinkedTo.Num() == 1))
        {
            const auto* SourcePin = Pin->LinkedTo[0];
            ProxyClass = SourcePin ? Cast<UClass>(SourcePin->PinType.PinSubCategoryObject.Get()) : nullptr;
        }
        Modify();
        ErrorMsg.Reset();

        for (int32 PinIndex = Pins.Num() - 1; PinIndex >= 0; --PinIndex)
        {
            auto N = Pins[PinIndex]->PinName;

            if (N != UEdGraphSchema_K2::PN_Self &&
                N != WorldContextPinName &&
                N != OutPutObjectPinName &&
                N != ClassPinName &&
                N != UEdGraphSchema_K2::PN_Execute &&
                N != UEdGraphSchema_K2::PN_Then &&
                N != NodeGuidStrName)
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
        const auto Idx = Pins.IndexOfByKey(OldPin);
        if (Idx != 0)
        {
            Pins.RemoveAt(Idx, 1, EAllowShrinking::No);
            Pins.Insert(OldPin, 0);
        }
    }
    else
    {
        CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    }

    if (const auto OldPin = FindPin(UEdGraphSchema_K2::PN_Then))
    {
        const auto Idx = Pins.IndexOfByKey(OldPin);
        if (Idx != 1)
        {
            Pins.RemoveAt(Idx, 1, EAllowShrinking::No);
            Pins.Insert(OldPin, 1);
        }
    }
    else
    {
        CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
    }

    if (IsValid(ProxyClass))
    {
        const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
        if (const auto* Function = GetFactoryFunction())
        {
            for (TFieldIterator<FProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
            {
                const auto* Param = *PropIt;

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
    {
        CreatePinsForClass(nullptr, true);
    }

    GenerateCustomOutputPins();

    for (auto& CurrentExtension : GetBlueprint()->GetExtensions())
    {
        if (CurrentExtension.IsA(UBtf_TaskForge::StaticClass()))
        {
            if (CurrentExtension.GetName().Contains(NodeGuid.ToString()))
            {
                TaskInstance = Cast<UBtf_TaskForge>(CurrentExtension);
                TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent PropertyChangedEvent)
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
            for (TFieldIterator<FProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
            {
                const auto* Param = *PropIt;
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
    {
        CreatePinsForClass(ProxyClass, true);
    }

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
    {
        NameArray.RemoveAll([](const FBtf_NameSelect& A) { return A.Name == NAME_None; });
    }

    auto CopyNameArray = NameArray;

    CopyNameArray.Sort([](const FBtf_NameSelect& A, const FBtf_NameSelect& B) { return GetTypeHash(A.Name) < GetTypeHash(B.Name); });

    auto UniquePred = [&Arr = NameArray](const FBtf_NameSelect& A, const FBtf_NameSelect& B)
    {
        if (A.Name != NAME_None && A.Name == B.Name)
        {
            const auto ID = Arr.FindLast(FBtf_NameSelect{B.Name});
            Arr.RemoveAt(ID, 1, EAllowShrinking::No);
            return true;
        }
        return false;
    };

    Algo::Unique(CopyNameArray, UniquePred);
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
    };
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
    };
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
    {
        ReconstructNode();
    }
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
                {
                    SpawnParam.Remove(PinNameSelect);
                }
            }
        }
    }

    Modify();
    auto NameStr = PinName.ToString();
    Pins.RemoveAll(
        [&NameStr](UEdGraphPin* InPin)
        {
            if (InPin->GetName().StartsWith(NameStr))
            {
                InPin->MarkAsGarbage();
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

void UBtf_ExtendConstructObject_K2Node::GenerateFactoryFunctionPins(UClass* InClass)
{
    const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
    if (const auto* Function = GetFactoryFunction())
    {
        auto AllPinsGood = true;
        auto PinsToHide = Get_PinsHiddenByDefault();

        FBlueprintEditorUtils::GetHiddenPinsForFunction(GetGraph(), Function, PinsToHide);
        for (TFieldIterator<FProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
        {
            auto* const Property = *PropIt;
            const auto IsFunctionInput = NOT Property->HasAnyPropertyFlags(CPF_OutParm) || Property->HasAnyPropertyFlags(CPF_ReferenceParm);
            if (NOT IsFunctionInput)
            { continue; }

            auto PinParams = FCreatePinParams{};
            PinParams.bIsReference = Property->HasAnyPropertyFlags(CPF_ReferenceParm) && IsFunctionInput;

            UEdGraphPin* Pin = nullptr;
            const auto N = Property->GetFName();
            if (N == WorldContextPinName || N == UEdGraphSchema_K2::PSC_Self)
            {
                if (auto* OldPin = FindPin(WorldContextPinName))
                {
                    Pin = OldPin;
                }
                if (auto* OldPin = FindPin(UEdGraphSchema_K2::PSC_Self))
                {
                    Pin = OldPin;
                }
                if (NOT OwnerContextPin)
                {
                    if (const auto* Prop = CastFieldChecked<FObjectProperty>(Property))
                    {
                        const auto BPClass = GetBlueprintClassFromNode();
                        SelfContext = BPClass->IsChildOf(Prop->PropertyClass);
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
            else if (N == ClassPinName)
            {
                if (auto* OldPin = GetClassPin())
                {
                    Pin = OldPin;
                    Pin->bHidden = false;
                }
            }

            if (NOT Pin)
            {
                Pin = CreatePin(EGPD_Input, NAME_None, Property->GetFName(), PinParams);
            }
            else
            {
                const auto Idx = Pins.IndexOfByKey(Pin);
                Pins.RemoveAt(Idx, 1, EAllowShrinking::No);
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
                {
                    AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
                }

                auto ParamValue = FString{};
                if (K2Schema->FindFunctionParameterDefaultValue(Function, Property, ParamValue))
                {
                    K2Schema->SetPinAutogeneratedDefaultValue(Pin, ParamValue);
                }
                else
                {
                    K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
                }

                if (PinsToHide.Contains(Pin->PinName))
                {
                    Pin->bHidden = true;
                }
            }

            AllPinsGood = AllPinsGood && PinGood;
        }

        if (AllPinsGood)
        {
            auto* ClassPin = FindPinChecked(ClassPinName);
            ClassPin->DefaultObject = InClass;
            ClassPin->DefaultValue.Empty();
            check(GetWorldContextPin());
        }
    }

    if (auto* OldPin = FindPin(OutPutObjectPinName))
    {
        const auto Idx = Pins.IndexOfByKey(OldPin);
        Pins.RemoveAt(Idx, 1, EAllowShrinking::No);
        Pins.Add(OldPin);
    }
    else
    {
        CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, InClass, OutPutObjectPinName);
    }
}

void UBtf_ExtendConstructObject_K2Node::GenerateSpawnParamPins(UClass* InClass)
{
    PinMetadataMap.Reset();

    const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
    const auto* ClassDefaultObject = InClass->GetDefaultObject(true);

    for (const auto ParamName : SpawnParam)
    {
        if (ParamName != NAME_None)
        {
            if (const auto* Property = InClass->FindPropertyByName(ParamName))
            {
                auto* Pin = CreatePin(EGPD_Input, NAME_None, ParamName);
                check(Pin);
                K2Schema->ConvertPropertyToPinType(Property, Pin->PinType);

                if (IsValid(ClassDefaultObject) && K2Schema->PinDefaultValueIsEditable(*Pin))
                {
                    auto DefaultValueAsString = FString{};
                    const auto DefaultValueSet =
                        FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(ClassDefaultObject), DefaultValueAsString, this);
                    check(DefaultValueSet);
                    K2Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValueAsString);
                }
                K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);

                if (const auto* MetaDataMap = Property->GetMetaDataMap())
                {
                    for (const auto& MetaDataKvp : *MetaDataMap)
                    {
                        PinMetadataMap.FindOrAdd(Pin->PinName).Add(MetaDataKvp.Key, MetaDataKvp.Value);
                    }
                }
            }
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GenerateAutoCallFunctionPins(UClass* InClass)
{
    for (const auto FnName : AutoCallFunction)
    {
        if (FnName != NAME_None)
        {
            CreatePinsForClassFunction(InClass, FnName, true);
            if (auto* ExecPin = FindPin(FnName, EGPD_Input))
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

void UBtf_ExtendConstructObject_K2Node::GenerateExecFunctionPins(UClass* InClass)
{
    for (const auto FnName : ExecFunction)
    {
        if (FnName != NAME_None)
        {
            CreatePinsForClassFunction(InClass, FnName, false);
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GenerateInputDelegatePins(UClass* InClass)
{
    for (auto DelegateName : InDelegate)
    {
        if (DelegateName != NAME_None)
        {
            if (const auto* Property = InClass->FindPropertyByName(DelegateName))
            {
                const auto* DelegateProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);

                auto DelegateNameStr = DelegateName.Name.ToString();

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

void UBtf_ExtendConstructObject_K2Node::GenerateOutputDelegatePins(UClass* InClass)
{
    const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
    for (auto DelegateName : OutDelegate)
    {
        if (DelegateName != NAME_None)
        {
            if (const auto* Property = InClass->FindPropertyByName(DelegateName))
            {
                const auto* DelegateProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);
                auto DelegateNameStr = DelegateName.Name.ToString();

                auto* ExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, DelegateName);
                ExecPin->PinToolTip = DelegateProperty->GetToolTipText().ToString();
                ExecPin->PinFriendlyName = FText::FromString(DelegateNameStr);

                if (const auto DelegateSignatureFunction = DelegateProperty->SignatureFunction)
                {
                    for (TFieldIterator<FProperty> PropIt(DelegateSignatureFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
                    {
                        const auto* Param = *PropIt;
                        const auto IsFunctionInput = NOT Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm);
                        if (IsFunctionInput)
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

void UBtf_ExtendConstructObject_K2Node::CreatePinsForClass(UClass* InClass, const bool FullRefresh)
{
    if (FullRefresh || NOT IsValid(InClass))
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

    GenerateFactoryFunctionPins(InClass);

    if (NOT IsValid(InClass))
    { return; }

    const auto* const ClassDefaultObject = InClass->GetDefaultObject(true);
    check(ClassDefaultObject);
    CollectSpawnParam(InClass, FullRefresh);

    check(ClassDefaultObject);
    GenerateSpawnParamPins(InClass);
    GenerateAutoCallFunctionPins(InClass);
    GenerateExecFunctionPins(InClass);
    GenerateInputDelegatePins(InClass);
    GenerateOutputDelegatePins(InClass);

    {
        const auto Pred = [&](const FBtf_NameSelect& A) { return A.Name != NAME_None && NOT FindPin(A.Name); };
        const auto ValidPropertyPred = [InCl = InClass](const FName& Name)
        {
            if (Name != NAME_None)
            {
                if (NOT InCl->FindPropertyByName(Name))
                {
                    return true;
                }
            }
            return false;
        };
        const auto ValidFunctionPred = [InCl = InClass](const FName& Name)
        {
            if (Name != NAME_None)
            {
                if (NOT InCl->FindPropertyByName(Name))
                {
                    return true;
                }
            }
            return false;
        };

        if (AllParam.Num())
        {
            auto Arr = AllParam.Array();
            if (const auto Count = Arr.RemoveAll(ValidPropertyPred))
            {
                AllParam = TSet<FName>(Arr);
            }

            AllParam.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
            AllParam.Shrink();
            SpawnParam.RemoveAll(Pred);
            SpawnParam.Shrink();
        }
        else
        {
            ResetPinByNames(SpawnParam);
        }

        if (AllDelegates.Num())
        {
            auto Arr = AllDelegates.Array();
            if (const auto Count = Arr.RemoveAll(ValidPropertyPred))
            {
                AllDelegates = TSet<FName>(Arr);
            }

            AllDelegates.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
            AllDelegates.Shrink();
            InDelegate.RemoveAll(Pred);
            OutDelegate.RemoveAll(Pred);
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
                auto Arr = AllFunctions.Array();
                if (const auto Count = Arr.RemoveAll(ValidFunctionPred))
                {
                    AllFunctions = TSet<FName>(Arr);
                }
            }
            AllFunctions.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
            AllFunctions.Shrink();

            AllFunctionsExec = AllFunctions;
            AutoCallFunction.RemoveAll(Pred);
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
            const auto PureFunctionPred = [InCl = InClass](const FName Name)
            {
                if (Name != NAME_None)
                {
                    if (const auto* Property = InCl->FindFunctionByName(Name))
                    {
                        return Property->HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Const);
                    }
                }
                return false;
            };

            {
                auto Arr = AllFunctionsExec.Array();
                if (const auto Count = Arr.RemoveAll(PureFunctionPred))
                {
                    AllFunctionsExec = TSet<FName>(Arr);
                }
            }

            ExecFunction.RemoveAll(Pred);
            ExecFunction.RemoveAll(PureFunctionPred);
            ExecFunction.Shrink();
        }
        else
        {
            ResetPinByNames(ExecFunction);
        }

        for (auto& It : AutoCallFunction)
        {
            It.SetAllExclude(AllFunctions, AutoCallFunction);
        }
        for (auto& It : ExecFunction)
        {
            It.SetAllExclude(AllFunctionsExec, ExecFunction);
        }
        for (auto& It : InDelegate)
        {
            It.SetAllExclude(AllDelegates, InDelegate);
        }
        for (auto& It : OutDelegate)
        {
            It.SetAllExclude(AllDelegates, OutDelegate);
        }
        for (auto& It : SpawnParam)
        {
            It.SetAllExclude(AllParam, SpawnParam);
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::CreatePinsForClassFunction(UClass* InClass, const FName FnName, const bool RetValPins)
{
    if (const auto* LocFunction = InClass->FindFunctionByName(FnName))
    {
        const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();

        auto* ExecPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, LocFunction->GetFName());
        ExecPin->PinToolTip = LocFunction->GetToolTipText().ToString();

        {
            auto FunctionNameStr = LocFunction->HasMetaData(FBlueprintMetadata::MD_DisplayName)
                ? LocFunction->GetMetaData(FBlueprintMetadata::MD_DisplayName)
                : FnName.ToString();

            if (FunctionNameStr.StartsWith(FNames_Helper::InPrefix))
            {
                FunctionNameStr.RemoveAt(0, FNames_Helper::InPrefix.Len());
            }
            if (FunctionNameStr.StartsWith(FNames_Helper::InitPrefix))
            {
                FunctionNameStr.RemoveAt(0, FNames_Helper::InitPrefix.Len());
            }
            ExecPin->PinFriendlyName = FText::FromString(FunctionNameStr);
        }

        auto AllPinsGood = true;
        for (TFieldIterator<FProperty> PropIt(LocFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
        {
            auto* Param = *PropIt;
            const auto IsFunctionInput =
                NOT Param->HasAnyPropertyFlags(CPF_ReturnParm) && (NOT Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm));
            const auto Direction = IsFunctionInput ? EGPD_Input : EGPD_Output;

            if (Direction == EGPD_Output && NOT RetValPins) continue;

            auto PinParams = UEdGraphNode::FCreatePinParams{};

            PinParams.bIsReference = Param->HasAnyPropertyFlags(CPF_ReferenceParm) && IsFunctionInput;

            const auto ParamFullName = FName(*FString::Printf(TEXT("%s_%s"), *FnName.ToString(), *Param->GetFName().ToString()));
            auto* Pin = CreatePin(Direction, NAME_None, ParamFullName, PinParams);
            const auto PinGood = (Pin && K2Schema->ConvertPropertyToPinType(Param, Pin->PinType));

            if (PinGood)
            {
                Pin->bDefaultValueIsIgnored = Param->HasAllPropertyFlags(CPF_ReferenceParm) &&
                    (NOT LocFunction->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm) || Pin->PinType.IsContainer());

                auto ParamValue = FString{};
                if (K2Schema->FindFunctionParameterDefaultValue(LocFunction, Param, ParamValue))
                {
                    K2Schema->SetPinAutogeneratedDefaultValue(Pin, ParamValue);
                }
                else
                {
                    K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
                }

                const auto& PinDisplayName = Param->HasMetaData(FBlueprintMetadata::MD_DisplayName)
                    ? Param->GetMetaData(FBlueprintMetadata::MD_DisplayName)
                    : Param->GetFName().ToString();

                if (PinDisplayName == FString(TEXT("ReturnValue")))
                {
                    const auto ReturnPropertyName = FString::Printf(TEXT("%s\n%s"), *FnName.ToString(), *PinDisplayName);
                    Pin->PinFriendlyName = FText::FromString(ReturnPropertyName);
                }
                else if (NOT PinDisplayName.IsEmpty())
                {
                    Pin->PinFriendlyName = FText::FromString(PinDisplayName);
                }
                else if (LocFunction->GetReturnProperty() == Param && LocFunction->HasMetaData(FBlueprintMetadata::MD_ReturnDisplayName))
                {
                    const auto ReturnPropertyName = FString::Printf(
                        TEXT("%s_%s"),
                        *FnName.ToString(),
                        *(LocFunction->GetMetaDataText(FBlueprintMetadata::MD_ReturnDisplayName).ToString()));
                    Pin->PinFriendlyName = FText::FromString(ReturnPropertyName);
                }
                Pin->PinToolTip = ParamFullName.ToString();
            }

            AllPinsGood = AllPinsGood && PinGood;
        }
        check(AllPinsGood);
    }
}

void UBtf_ExtendConstructObject_K2Node::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);
    if (NOT IsValid(ProxyClass))
    {
        CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: ProxyClass nullptr error. @@"), this);
        return;
    }

    const auto* Schema = CompilerContext.GetSchema();
    check(SourceGraph && Schema);
    auto IsErrorFree = true;

    // Create a call to factory the proxy object
    auto* const CreateProxyObject_Node = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    CreateProxyObject_Node->FunctionReference.SetExternalMember(ProxyFactoryFunctionName, ProxyFactoryClass);
    CreateProxyObject_Node->AllocateDefaultPins();
    if (NOT IsValid(CreateProxyObject_Node->GetTargetFunction()))
    {
        const auto ClassName = IsValid(ProxyFactoryClass) ? FText::FromString(ProxyFactoryClass->GetName()) : LOCTEXT("MissingClassString", "Unknown Class");
        const auto FormattedMessage = FText::Format(
                LOCTEXT("AsyncTaskErrorFmt", "BaseAsyncTask: Missing function {0} from class {1} for async task @@"),
                FText::FromString(ProxyFactoryFunctionName.GetPlainNameString()),
                ClassName)
                .ToString();

        CompilerContext.MessageLog.Error(*FormattedMessage, this);
        return;
    }

    IsErrorFree &= CompilerContext
                        .MovePinLinksToIntermediate(
                            *FindPinChecked(UEdGraphSchema_K2::PN_Execute),
                            *CreateProxyObject_Node->FindPinChecked(UEdGraphSchema_K2::PN_Execute))
                        .CanSafeConnect();
    if (SelfContext)
    {
        auto* const Self_Node = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(this, SourceGraph);
        Self_Node->AllocateDefaultPins();
        auto* SelfPin = Self_Node->FindPinChecked(UEdGraphSchema_K2::PN_Self);
        IsErrorFree &= Schema->TryCreateConnection(SelfPin, CreateProxyObject_Node->FindPinChecked(WorldContextPinName));
        if (NOT IsErrorFree)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Pin Self Context Error. @@"), this);
        }
    }

    for (auto* CurrentPin : Pins)
    {
        if (CurrentPin->PinName != UEdGraphSchema_K2::PSC_Self && FNodeHelper::ValidDataPin(CurrentPin, EGPD_Input))
        {
            if (auto* DestPin = CreateProxyObject_Node->FindPin(CurrentPin->PinName))
            {
                IsErrorFree &= CompilerContext.CopyPinLinksToIntermediate(*CurrentPin, *DestPin).CanSafeConnect();
                if (NOT IsErrorFree)
                {
                    CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Create a call to factory the proxy object error. @@"), this);
                }
            }
        }
    }

    CreateProxyObject_Node->FindPin(NodeGuidStrName)->DefaultValue = NodeGuid.ToString();

    // Expose Async Task Proxy object
    auto* const PinProxyObject = CreateProxyObject_Node->GetReturnValuePin();
    auto* Cast_Output = PinProxyObject;
    if (PinProxyObject->PinType.PinSubCategoryObject.Get() != ProxyClass)
    {
        auto* CastNode = CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, SourceGraph);
        CastNode->SetPurity(true);
        CastNode->TargetType = ProxyClass;
        CastNode->AllocateDefaultPins();
        IsErrorFree &= Schema->TryCreateConnection(PinProxyObject, CastNode->GetCastSourcePin());
        Cast_Output = CastNode->GetCastResultPin();
        check(Cast_Output);
    }
    auto* const OutputObjectPin = FindPinChecked(OutPutObjectPinName, EGPD_Output);
    IsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*OutputObjectPin, *Cast_Output).CanSafeConnect();
    if (NOT IsErrorFree)
    {
        CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: OutObjectPin Error. @@"), this);
    }

    // Gather output parameters and pair them with local variables
    auto VariableOutputs = TArray<FNodeHelper::FOutputPinAndLocalVariable>{};
    for (auto* CurrentPin : Pins)
    {
        if (OutputObjectPin != CurrentPin && FNodeHelper::ValidDataPin(CurrentPin, EGPD_Output))
        {
            const auto& PinType = CurrentPin->PinType;

            bool bIsCustomPin = false;
            for (const auto& CustomPin : CustomPins)
            {
                if (CustomPin.PayloadType)
                {
                    for (TFieldIterator<FProperty> It(CustomPin.PayloadType); It; ++It)
                    {
                        FProperty* Property = *It;

                        if (!Property->HasAnyPropertyFlags(CPF_Parm) &&
                            Property->HasAllPropertyFlags(CPF_BlueprintVisible))
                        {
                            FName PayloadPinName = Property->GetFName();

                            if (CurrentPin->PinName == PayloadPinName)
                            {
                                bIsCustomPin = true;
                                break;
                            }
                        }
                    }
                }

                if (bIsCustomPin)
                { break; }
            }

            if (bIsCustomPin)
            { continue; }

            auto* TempVarOutput = CompilerContext.SpawnInternalVariable(
                    this,
                    PinType.PinCategory,
                    PinType.PinSubCategory,
                    PinType.PinSubCategoryObject.Get(),
                    PinType.ContainerType,
                    PinType.PinValueType);

            IsErrorFree &= TempVarOutput->GetVariablePin() != nullptr;
            IsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *TempVarOutput->GetVariablePin()).CanSafeConnect();
            VariableOutputs.Add(FNodeHelper::FOutputPinAndLocalVariable(CurrentPin, TempVarOutput));
        }
    }

    const auto IsValidFuncName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid);

    auto* OriginalThenPin = FindPinChecked(UEdGraphSchema_K2::PN_Then);
    auto* LastThenPin = CreateProxyObject_Node->FindPinChecked(UEdGraphSchema_K2::PN_Then);
    auto* ValidateProxyNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
    ValidateProxyNode->AllocateDefaultPins();
    {
        auto* IsValidFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

        IsValidFuncNode->FunctionReference.SetExternalMember(IsValidFuncName, UKismetSystemLibrary::StaticClass());
        IsValidFuncNode->AllocateDefaultPins();
        auto* IsValidInputPin = IsValidFuncNode->FindPinChecked(TEXT("Object"));

        IsErrorFree &= Schema->TryCreateConnection(PinProxyObject, IsValidInputPin);
        IsErrorFree &= Schema->TryCreateConnection(IsValidFuncNode->GetReturnValuePin(), ValidateProxyNode->GetConditionPin());
        IsErrorFree &= Schema->TryCreateConnection(LastThenPin, ValidateProxyNode->GetExecPin());
        IsErrorFree &= CompilerContext.CopyPinLinksToIntermediate(*OriginalThenPin, *ValidateProxyNode->GetElsePin()).CanSafeConnect();

        LastThenPin = ValidateProxyNode->GetThenPin();
    }

    // Create InDelegate Assign
    for (auto DelegateName : InDelegate)
    {
        if (DelegateName == NAME_None) continue;

        if (auto* Property = ProxyClass->FindPropertyByName(DelegateName))
        {
            if (auto* CurrentProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);
                CurrentProperty && CurrentProperty->GetFName() == DelegateName)
            {
                if (auto* InDelegatePin = FindPinChecked(DelegateName, EGPD_Input);
                    InDelegatePin->LinkedTo.Num() != 0)
                {
                    auto* AssignDelegate = CompilerContext.SpawnIntermediateNode<UK2Node_AssignDelegate>(this, SourceGraph);
                    AssignDelegate->SetFromProperty(CurrentProperty, false, CurrentProperty->GetOwnerClass());
                    AssignDelegate->AllocateDefaultPins();

                    IsErrorFree &= Schema->TryCreateConnection(AssignDelegate->FindPinChecked(UEdGraphSchema_K2::PN_Self), Cast_Output);
                    IsErrorFree &= Schema->TryCreateConnection(LastThenPin, AssignDelegate->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
                    auto* DelegatePin = AssignDelegate->GetDelegatePin();

                    IsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*InDelegatePin, *DelegatePin).CanSafeConnect();
                    if (NOT IsErrorFree)
                    {
                        CompilerContext.MessageLog.Error(*LOCTEXT("Invalid_DelegateProperties", "Error @@").ToString(), this);
                    }
                    LastThenPin = AssignDelegate->FindPinChecked(UEdGraphSchema_K2::PN_Then);
                }
            }
        }
        else
        {
            IsErrorFree = false;
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
        }
    }

    // Create OutDelegate
    for (auto DelegateName : OutDelegate)
    {
        if (DelegateName == NAME_None) continue;

        if (auto* Property = ProxyClass->FindPropertyByName(DelegateName))
        {
            if (auto* CurrentProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);
                CurrentProperty && CurrentProperty->GetFName() == DelegateName)
            {
                if (FindPin(CurrentProperty->GetFName()))
                {
                    IsErrorFree &= FNodeHelper::HandleDelegateImplementation(
                        CurrentProperty,
                        VariableOutputs,
                        Cast_Output,
                        LastThenPin,
                        this,
                        SourceGraph,
                        CompilerContext);
                }
                else
                {
                    IsErrorFree = false;
                }
                if (NOT IsErrorFree)
                {
                    CompilerContext.MessageLog.Error(*LOCTEXT("Invalid_DelegateProperties", "@@").ToString(), this);
                }
            }
        }
        else
        {
            IsErrorFree = false;
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
        }
    }

    if (NOT IsErrorFree)
    {
        CompilerContext.MessageLog.Error(
            TEXT("ExtendConstructObject: For each delegate define event, connect it to delegate and implement a chain of assignments error. @@"),
            this);
    }
    if (CreateProxyObject_Node->FindPinChecked(UEdGraphSchema_K2::PN_Then) == LastThenPin)
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("MissingDelegateProperties", "ExtendConstructObject: Proxy has no delegates defined. @@").ToString(), this);
        return;
    }

    IsErrorFree &= ConnectSpawnProperties(ProxyClass, Schema, CompilerContext, SourceGraph, LastThenPin, PinProxyObject);
    if (NOT IsErrorFree)
    {
        CompilerContext.MessageLog.Error(TEXT("ConnectSpawnProperties error. @@"), this);
    }

    // Create the custom pins
    if (NOT CustomPins.IsEmpty())
    {
        auto* DelegateProperty = ProxyClass->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UBtf_TaskForge, OnCustomPinTriggered));
        auto* MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(DelegateProperty);

        if (NOT MulticastDelegateProperty)
        {
            CompilerContext.MessageLog.Error(*FString::Printf(TEXT("Failed to find delegate property for %s."), *GetNodeTitle(ENodeTitleType::FullTitle).ToString()));
            BreakAllNodeLinks();
            return;
        }

        const auto Success = FNodeHelper::HandleCustomPinsImplementation(
            MulticastDelegateProperty,
            Cast_Output,
            LastThenPin,
            this,
            SourceGraph,
            CustomPins,
            CompilerContext
        );

        if (NOT Success)
        {
            CompilerContext.MessageLog.Error(*FString::Printf(TEXT("Failed to handle delegate for %s."), *GetNodeTitle(ENodeTitleType::FullTitle).ToString()));
        }
    }

    // Create a call to activate the proxy object
    for (auto FunctionName : AutoCallFunction)
    {
        if (FunctionName == NAME_None) continue;
        {
            const auto* FindFunc = ProxyClass->FindFunctionByName(FunctionName);
            if (NOT IsValid(FindFunc))
            {
                IsErrorFree = false;
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
                continue;
            }
        }

        auto* const CallFunction_Node = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
        CallFunction_Node->FunctionReference.SetExternalMember(FunctionName, ProxyClass);
        CallFunction_Node->AllocateDefaultPins();

        auto* ActivateCallSelfPin = Schema->FindSelfPin(*CallFunction_Node, EGPD_Input);
        check(ActivateCallSelfPin);
        IsErrorFree &= Schema->TryCreateConnection(Cast_Output, ActivateCallSelfPin);
        if (NOT IsErrorFree)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Create a call to activate the proxy object error. @@"), this);
        }

        const auto IsPureFunc = CallFunction_Node->IsNodePure();
        if (NOT IsPureFunc)
        {
            auto* ActivateExecPin = CallFunction_Node->FindPinChecked(UEdGraphSchema_K2::PN_Execute);
            auto* ActivateThenPin = CallFunction_Node->FindPinChecked(UEdGraphSchema_K2::PN_Then);
            IsErrorFree &= Schema->TryCreateConnection(LastThenPin, ActivateExecPin);
            if (NOT IsErrorFree)
            {
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Create a call to activate the proxy object error. @@"), this);
            }
            LastThenPin = ActivateThenPin;
        }

        auto* EventSignature = CallFunction_Node->GetTargetFunction();
        check(EventSignature);
        for (TFieldIterator<FProperty> PropIt(EventSignature); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
        {
            const auto* Param = *PropIt;

            const auto IsFunctionInput = NOT Param->HasAnyPropertyFlags(CPF_ReturnParm) &&
                (NOT Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm));

            const auto Direction = IsFunctionInput ? EGPD_Input : EGPD_Output;

            if (auto* InNodePin = FindParamPin(FunctionName.Name.ToString(), Param->GetFName(), Direction))
            {
                auto* InFunctionPin = CallFunction_Node->FindPinChecked(Param->GetFName(), Direction);
                if (IsFunctionInput)
                {
                    IsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*InNodePin, *InFunctionPin).CanSafeConnect();
                }
                else
                {
                    auto* OutputPair = VariableOutputs.FindByKey(InNodePin);
                    check(OutputPair);

                    auto* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
                    AssignNode->AllocateDefaultPins();
                    IsErrorFree &= Schema->TryCreateConnection(LastThenPin, AssignNode->GetExecPin());

                    IsErrorFree &= Schema->TryCreateConnection(OutputPair->TempVar->GetVariablePin(), AssignNode->GetVariablePin());
                    AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());
                    IsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), InFunctionPin);
                    AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

                    LastThenPin = AssignNode->GetThenPin();
                }
            }
            else
            {
                IsErrorFree = false;
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
            }
        }

        if (NOT IsErrorFree)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Create a call to activate the proxy object error. @@"), this);
        }
    }

    // Create a call to Expand Functions the proxy object
    for (auto FunctionName : ExecFunction)
    {
        if (FunctionName == NAME_None) continue;
        {
            if (const auto* FindFunc = ProxyClass->FindFunctionByName(FunctionName);
                NOT FindFunc)
            {
                IsErrorFree = false;
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
                continue;
            }
        }

        auto* ExecPin = FindPinChecked(FunctionName, EGPD_Input);
        auto* IsValidFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

        auto* IfThenElse = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);

        IsValidFuncNode->FunctionReference.SetExternalMember(IsValidFuncName, UKismetSystemLibrary::StaticClass());
        IsValidFuncNode->AllocateDefaultPins();
        IfThenElse->AllocateDefaultPins();

        auto* IsValidInputPin = IsValidFuncNode->FindPinChecked(TEXT("Object"));
        IsErrorFree &= Schema->TryCreateConnection(Cast_Output, IsValidInputPin);
        IsErrorFree &= Schema->TryCreateConnection(IsValidFuncNode->GetReturnValuePin(), IfThenElse->GetConditionPin());

        IsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*ExecPin, *IfThenElse->GetExecPin()).CanSafeConnect();

        auto* const CallFunction_Node = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
        CallFunction_Node->FunctionReference.SetExternalMember(FunctionName, ProxyClass);
        CallFunction_Node->AllocateDefaultPins();

        if (CallFunction_Node->IsNodePure())
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need To Refresh Node. @@"), this);
            continue;
        }

        auto* CallSelfPin = Schema->FindSelfPin(*CallFunction_Node, EGPD_Input);
        check(CallSelfPin);

        IsErrorFree &= Schema->TryCreateConnection(Cast_Output, CallSelfPin);

        auto* FunctionExecPin = CallFunction_Node->FindPinChecked(UEdGraphSchema_K2::PN_Execute);

        check(FunctionExecPin && ExecPin);

        IsErrorFree &= Schema->TryCreateConnection(IfThenElse->GetThenPin(), FunctionExecPin);

        auto* EventSignature = CallFunction_Node->GetTargetFunction();
        for (TFieldIterator<FProperty> PropIt(EventSignature); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
        {
            const auto* Param = *PropIt;
            const auto IsFunctionInput = NOT Param->HasAnyPropertyFlags(CPF_ReturnParm) &&
                (NOT Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm));
            if (IsFunctionInput)
            {
                if (auto* InNodePin = FindParamPin(FunctionName.Name.ToString(), Param->GetFName(), EGPD_Input))
                {
                    auto* InFunctionPin = CallFunction_Node->FindPinChecked(Param->GetFName(), EGPD_Input);

                    IsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*InNodePin, *InFunctionPin).CanSafeConnect();
                }
                else
                {
                    IsErrorFree = false;
                    CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
                }
            }
        }
    }

    if (NOT IsErrorFree)
    {
        CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Create a call to Expand Functions the proxy object error. @@"), this);
    }

    // Move the connections from the original node then pin to the last internal then pin
    IsErrorFree &= CompilerContext.CopyPinLinksToIntermediate(*OriginalThenPin, *LastThenPin).CanSafeConnect();
    if (NOT IsErrorFree)
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "ExtendConstructObject: Internal connection error. @@").ToString(), this);
    }

    SpawnParam.Shrink();

    BreakAllNodeLinks();
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

                if (IsValid(ClassToSpawn->ClassDefaultObject))
                {
                    auto DefaultValueAsString = FString{};
                    FBlueprintEditorUtils::PropertyValueToString(
                        Property,
#if ENGINE_MINOR_VERSION < 3
                        reinterpret_cast<uint8*>(ClassToSpawn->ClassDefaultObject),
#else
                        reinterpret_cast<uint8*>(ClassToSpawn->ClassDefaultObject.Get()),
#endif
                        DefaultValueAsString,
                        this);

                    if (DefaultValueAsString == SpawnVarPin->DefaultValue) continue;
                }
            }

            if (const auto* SetByNameFunction = Schema->FindSetVariableByNameFunction(SpawnVarPin->PinType))
            {
                auto* SetVarNode = SpawnVarPin->PinType.IsArray()
                    ? CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph)
                    : CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

                SetVarNode->SetFromFunction(SetByNameFunction);
                SetVarNode->AllocateDefaultPins();

                // Connect this node into the exec chain
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

                // Connect the new actor to the 'object' pin
                auto* ObjectPin = SetVarNode->FindPinChecked(ObjectParamName);
                SpawnedActorReturnPin->MakeLinkTo(ObjectPin);

                // Fill in literal for 'property name' pin - name of pin is property name
                auto* PropertyNamePin = SetVarNode->FindPinChecked(PropertyNameParamName);
                PropertyNamePin->DefaultValue = SpawnVarPin->PinName.ToString();

                auto* ValuePin = SetVarNode->FindPinChecked(ValueParamName);
                if (SpawnVarPin->LinkedTo.Num() == 0 &&
                    SpawnVarPin->DefaultValue != FString() &&
                    SpawnVarPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte &&
                    SpawnVarPin->PinType.PinSubCategoryObject.IsValid() &&
                    SpawnVarPin->PinType.PinSubCategoryObject->IsA<UEnum>())
                {
                    // Pin is an enum, we need to alias the enum value to an int:
                    auto* EnumLiteralNode = CompilerContext.SpawnIntermediateNode<UK2Node_EnumLiteral>(this, SourceGraph);
                    EnumLiteralNode->Enum = CastChecked<UEnum>(SpawnVarPin->PinType.PinSubCategoryObject.Get());
                    EnumLiteralNode->AllocateDefaultPins();
                    EnumLiteralNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(ValuePin);

                    auto* InPin = EnumLiteralNode->FindPinChecked(UK2Node_EnumLiteral::GetEnumInputPinName());
                    InPin->DefaultValue = SpawnVarPin->DefaultValue;
                }
                else
                {
                    // For non-array struct pins that are not linked, transfer the pin type so that the node will expand an auto-ref that will assign the value by-ref.
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
                        // Move connection from the variable pin on the spawn node to the 'value' pin
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

    // WORKAROUND, so we can create delegate from nonexistent function by avoiding check at expanding step
    // instead simply: Schema->TryCreateConnection(AddDelegateNode->GetDelegatePin(), CurrentCENode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName));
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
    for (TFieldIterator<FProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
    {
        const auto* Param = *PropIt;
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
    for (const auto& OutputPair : VariableOutputs) // CREATE CHAIN OF ASSIGMENTS
    {
        auto PinNameStr = OutputPair.OutputPin->PinName.ToString();

        if (NOT PinNameStr.StartsWith(DelegateNameStr))
        { continue; }

        PinNameStr.RemoveAt(0, DelegateNameStr.Len() + 1);

        auto* PinWithData = CurrentCeNode->FindPinChecked(FName(*PinNameStr));

        if (NOT PinWithData)
        { return false; }

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

        // We need to assign the signature for the proper pins to generate before we connect them
        IsErrorFree &= FNodeHelper::CreateDelegateForNewFunction(
            AddDelegateNode->GetDelegatePin(),
            CurrentCeNode->GetFunctionName(),
            CurrentNode,
            SourceGraph,
            CompilerContext);
        IsErrorFree &= FNodeHelper::CopyEventSignature(CurrentCeNode, AddDelegateNode->GetDelegateSignature(), Schema);

        auto* PinNamePin = CurrentCeNode->FindPin(TEXT("PinName"));
        auto* DataPin = CurrentCeNode->FindPin(TEXT("Data"));

        // Create a switch node for routing to the correct custom pin
        auto* SwitchNode = CompilerContext.SpawnIntermediateNode<UK2Node_SwitchName>(CurrentNode, SourceGraph);
        auto ConvertedOutputNames = TArray<FName>{};
        for (auto& CurrentPin : OutputNames)
        {
            ConvertedOutputNames.Add(FName(CurrentPin.PinName));
        }
        SwitchNode->PinNames = ConvertedOutputNames;
        SwitchNode->AllocateDefaultPins();

        // Connect the custom event to the switch node
        Schema->TryCreateConnection(CurrentCeNode->FindPinChecked(UEdGraphSchema_K2::PN_Then), SwitchNode->GetExecPin());
        Schema->TryCreateConnection(PinNamePin, SwitchNode->GetSelectionPin());

        // For each custom output pin
        for (int32 i = 0; i < OutputNames.Num(); ++i)
        {
            const auto& OutputName = FName(OutputNames[i].PinName);
            if (auto* SwitchCasePin = SwitchNode->FindPin(OutputName.ToString()))
            {
                // Handle payload extraction if this pin has a payload type
                if (OutputNames[i].PayloadType)
                {
                    // Create GetInstancedStructValue node
                    auto* GetInstancedStructNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(CurrentNode, SourceGraph);
                    GetInstancedStructNode->SetFromFunction(
                        UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(
                            GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, GetInstancedStructValue)));
                    GetInstancedStructNode->AllocateDefaultPins();

                    // Connect the data pin to GetInstancedStructValue
                    Schema->TryCreateConnection(DataPin, GetInstancedStructNode->FindPin(TEXT("InstancedStruct")));
                    Schema->TryCreateConnection(SwitchCasePin, GetInstancedStructNode->GetExecPin());

                    // Create BreakStruct node
                    auto* BreakStructNode = CompilerContext.SpawnIntermediateNode<UK2Node_BreakStruct>(CurrentNode, SourceGraph);
                    BreakStructNode->StructType = OutputNames[i].PayloadType;
                    BreakStructNode->AllocateDefaultPins();
                    BreakStructNode->bMadeAfterOverridePinRemoval = true;

                    // Connect GetInstancedStructValue to BreakStruct
                    UEdGraphPin* ValuePin = GetInstancedStructNode->FindPin(TEXT("Value"));
                    UEdGraphPin* StructInputPin = BreakStructNode->FindPin(OutputNames[i].PayloadType->GetFName());
                    Schema->TryCreateConnection(ValuePin, StructInputPin);

                    // Connect the broken out values to the output pins
                    for (TFieldIterator<FProperty> It(OutputNames[i].PayloadType); It; ++It)
                    {
                        FProperty* Property = *It;
                        if (!Property->HasAnyPropertyFlags(CPF_Parm) &&
                            Property->HasAllPropertyFlags(CPF_BlueprintVisible))
                        {
                            UEdGraphPin* BreakPin = BreakStructNode->FindPin(Property->GetFName());
                            if (BreakPin)
                            {
                                FName PayloadPinName = Property->GetFName();

                                if (UEdGraphPin* NodeOutputPin = CurrentNode->FindPin(PayloadPinName))
                                {
                                    CompilerContext.MovePinLinksToIntermediate(*NodeOutputPin, *BreakPin);
                                }
                            }
                        }
                    }

                    // Connect execution output
                    if (auto* NodeOutputPin = CurrentNode->FindPin(OutputName))
                    {
                        UEdGraphPin* ValidPin = GetInstancedStructNode->FindPin(TEXT("Valid"));
                        CompilerContext.MovePinLinksToIntermediate(*NodeOutputPin, *ValidPin);
                    }
                }
                else
                {
                    // No payload, just connect the execution pin
                    if (auto* NodeOutputPin = CurrentNode->FindPin(OutputName))
                    {
                        CompilerContext.MovePinLinksToIntermediate(*NodeOutputPin, *SwitchCasePin);
                    }
                }
            }
        }
    }

    return IsErrorFree;
}

void UBtf_ExtendConstructObject_K2Node::CollectSpawnParam(UClass* InClass, const bool FullRefresh)
{
    const auto InPrefix = FString::Printf(TEXT("In_"));
    const auto OutPrefix = FString::Printf(TEXT("Out_"));
    const auto InitPrefix = FString::Printf(TEXT("Init_"));

    //params
    {
        for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
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
                    //force add ExposedToSpawn Param
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

    //functions
    {
        for (TFieldIterator<UField> It(InClass); It; ++It)
        {
            if (const auto* LocFunction = Cast<UFunction>(*It))
            {
                if (LocFunction->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public) &&
                    NOT LocFunction->HasAnyFunctionFlags(
                        FUNC_Static | FUNC_UbergraphFunction | FUNC_Delegate | FUNC_Private | FUNC_Protected | FUNC_EditorOnly) &&
                    NOT LocFunction->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) &&
                    NOT LocFunction->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction) &&
                    NOT FObjectEditorUtils::IsFunctionHiddenFromClass(LocFunction, InClass))
                {
                    const auto FunctionName = LocFunction->GetFName();

                    const auto OldNum = AllFunctions.Num();
                    AllFunctions.Add(FunctionName);
                    //force add InitFunction
                    if (LocFunction->GetBoolMetaData(FName("ExposeAutoCall")) || FunctionName.ToString().StartsWith(InitPrefix))
                    {
                        AutoCallFunction.AddUnique(FBtf_NameSelect{FunctionName});
                    }
                    const auto NewFunction = OldNum - AllFunctions.Num() != 0;
                    if (NewFunction)
                    {
                        if (FunctionName.ToString().StartsWith(InPrefix) && NOT LocFunction->HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Const))
                        {
                            if (FullRefresh)
                            {
                                ExecFunction.AddUnique(FBtf_NameSelect{FunctionName});
                            }
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

    //delegates
    {
        const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();
        const auto GraphType = K2Schema->GetGraphType(GetGraph());
        const auto AllowOutDelegate = GraphType != EGraphType::GT_Function;

        for (TFieldIterator<FProperty> PropertyIt(InClass); PropertyIt; ++PropertyIt)
        {
            if (const auto* DelegateProperty = CastField<FMulticastDelegateProperty>(*PropertyIt))
            {
                if (DelegateProperty->HasAnyPropertyFlags(FUNC_Private | CPF_Protected | FUNC_EditorOnly) ||
                    NOT DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintAssignable) ||
                    DelegateProperty->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) ||
                    DelegateProperty->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction))
                {
                    continue;
                }

                if (const auto LocFunction = DelegateProperty->SignatureFunction)
                {
                    if (NOT LocFunction->HasAllFunctionFlags(FUNC_Public) ||
                        LocFunction->HasAnyFunctionFlags(
                            FUNC_Static | FUNC_BlueprintPure | FUNC_Const | FUNC_UbergraphFunction | FUNC_Private | FUNC_Protected | FUNC_EditorOnly) ||
                        LocFunction->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) ||
                        LocFunction->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction))
                    {
                        continue;
                    }
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
                    {
                        InDelegate.AddUnique(FBtf_NameSelect{DelegateName});
                    }
                    else if (DelegateName.ToString().StartsWith(OutPrefix) && AllowOutDelegate)
                    {
                        OutDelegate.AddUnique(FBtf_NameSelect{DelegateName});
                    }
                    else
                    {
                        continue;
                    }
                }

                if (DelegateName != NAME_None)
                {
                    auto AllPinsGood = true;
                    auto DelegateNameStr = DelegateName.ToString();

                    if (DelegateNameStr.StartsWith(OutPrefix)) //check Properties
                    {
                        DelegateNameStr.RemoveAt(0, OutPrefix.Len());
                        if (const auto DelegateSignatureFunction = DelegateProperty->SignatureFunction)
                        {
                            for (TFieldIterator<FProperty> PropIt(DelegateSignatureFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
                            {
                                const auto ParamName = FName(*FString::Printf(TEXT("%s_%s"), *DelegateNameStr, *(*PropIt)->GetFName().ToString()));
                                AllPinsGood &= FindPin(ParamName, EGPD_Output) != nullptr;
                            }

                            for (const auto* Pin : Pins)
                            {
                                auto PinNameStr = Pin->GetName();
                                if (PinNameStr.StartsWith(DelegateNameStr))
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
                        {
                            DelegateNameStr.RemoveAt(0, InPrefix.Len());
                        }
                        if (DelegateNameStr.StartsWith(OutPrefix))
                        {
                            DelegateNameStr.RemoveAt(0, OutPrefix.Len());
                        }

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
                //todo not Multicast DelegateProperty
            }
        }
    }

    Algo::Sort(
        AutoCallFunction,
        [&](const FName& A, const FName& B)
        {
            const auto AStr = A.ToString();
            const auto BStr = B.ToString();
            const auto A_Init = AStr.StartsWith(InitPrefix);
            const auto B_Init = BStr.StartsWith(InitPrefix);
            return (A_Init && NOT B_Init) ||
                ((A_Init && B_Init) ? UBtf_ExtendConstructObject_Utils::LessSuffix(A, AStr, B, BStr) : A.FastLess(B));
        });
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------