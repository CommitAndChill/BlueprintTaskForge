//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "K2Node_Blueprint_Template.h"

#include "BlueprintTaskTemplate.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet2/BlueprintEditorUtils.h"

//++CK
#include "K2Node_CallFunction.h"
#include "SGraphPanel.h"
#include "Internationalization/BreakIterator.h"
//--CK

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_Blueprint_Template::UK2Node_Blueprint_Template(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UBlueprintTaskTemplate, BlueprintTaskTemplate);
    ProxyFactoryClass = UBlueprintTaskTemplate::StaticClass();
    OutPutObjectPinName = FName(TEXT("TaskObject"));
    //AutoCallFunctions.Add(GET_FUNCTION_NAME_CHECKED(UBlueprintTaskTemplate, Init_Activate));
}

void UK2Node_Blueprint_Template::HideClassPin() const
{
    UEdGraphPin* ClassPin = FindPinChecked(ClassPinName);
    ClassPin->DefaultObject = ProxyClass;
    ClassPin->DefaultValue.Empty();
    ClassPin->bDefaultValueIsReadOnly = true;
    ClassPin->bNotConnectable = true;
    ClassPin->bHidden = true;
}

void UK2Node_Blueprint_Template::RegisterBlueprintAction(UClass* TargetClass, FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    const FString Name = TargetClass->GetName();

    if (!TargetClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated) && !Name.Contains(FNames_Helper::SkelPrefix) &&
        !Name.Contains(FNames_Helper::ReinstPrefix) && !Name.Contains(FNames_Helper::DeadclassPrefix))
    {
        UClass* NodeClass = GetClass();
        auto Lambda = [NodeClass, TargetClass](const UFunction* FactoryFunc) -> UBlueprintNodeSpawner*
        {
            auto CustomizeTimelineNodeLambda = [TargetClass](UEdGraphNode* NewNode, bool bIsTemplateNode, const TWeakObjectPtr<UFunction> FunctionPtr)
            {
                UK2Node_Blueprint_Template* AsyncTaskNode = CastChecked<UK2Node_Blueprint_Template>(NewNode);
                if (FunctionPtr.IsValid())
                {
                    UFunction* Func = FunctionPtr.Get();
                    AsyncTaskNode->ProxyFactoryFunctionName = Func->GetFName();
                    //AsyncTaskNode->ProxyFactoryClass = Func->GetOuterUClass();
                    AsyncTaskNode->ProxyClass = TargetClass;
                }
            };

            UBlueprintNodeSpawner* NodeSpawner = UBlueprintFunctionNodeSpawner::Create(FactoryFunc);
            NodeSpawner->NodeClass = NodeClass;
            if (TargetClass && TargetClass->HasMetaData(TEXT("Category")))
            {
                NodeSpawner->DefaultMenuSignature.Category = FText::FromString(TargetClass->GetMetaData(TEXT("Category")));
            }

            const TWeakObjectPtr<UFunction> FunctionPtr = MakeWeakObjectPtr(const_cast<UFunction*>(FactoryFunc));
            FBlueprintActionUiSpec& MenuSignature = NodeSpawner->DefaultMenuSignature;

//++CK
            if (TargetClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
            {
                FString LocName = TargetClass->GetName();
                LocName.RemoveFromEnd(FNames_Helper::CompiledFromBlueprintSuffix);

                if (const UBlueprintTaskTemplate* TargetClassAsBlueprintTask = Cast<UBlueprintTaskTemplate>(TargetClass->ClassDefaultObject))
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
            }
//--CK

            NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateLambda(CustomizeTimelineNodeLambda, FunctionPtr);
            return NodeSpawner;
        };

        /*if (const UObject* RegistrarTarget = ActionRegistrar.GetActionKeyFilter()) //... todo: RegisterBlueprintAction ActionRegistrar.GetActionKeyFilter()
        {
            if (const UClass* TarClass = Cast<UClass>(RegistrarTarget))
            {
                if (!TarClass->HasAnyClassFlags(CLASS_Abstract) && !TarClass->IsChildOf(TargetClass))
                {
                    for (TFieldIterator<UFunction> FuncIt(TarClass); FuncIt; ++FuncIt)
                    {
                        UFunction* Function = *FuncIt;
                        if (!Function->HasAnyFunctionFlags(FUNC_Static)) continue;
                        if (CastField<FObjectProperty>(Function->GetReturnProperty()) == nullptr) continue;

                        if (UBlueprintNodeSpawner* NewAction = Lambda(Function))
                        {
                            ActionRegistrar.AddBlueprintAction(Function, NewAction);
                        }
                    }
                }
            }
        }
        else*/
        {
            for (TFieldIterator<UFunction> FuncIt(TargetClass); FuncIt; ++FuncIt)
            {
                const UFunction* const Function = *FuncIt;
                if (!Function->HasAnyFunctionFlags(FUNC_Static)) continue;
                if (CastField<FObjectProperty>(Function->GetReturnProperty()) == nullptr) continue;

                if (UBlueprintNodeSpawner* NewAction = Lambda(Function))
                {
                    ActionRegistrar.AddBlueprintAction(Function, NewAction);
                }
            }
        }
    }
}

void UK2Node_Blueprint_Template::AllocateDefaultPins()
{
    Super::AllocateDefaultPins();
    check(GetWorldContextPin());
    HideClassPin();

    UK2Node::AllocateDefaultPins();
    GetGraph()->NotifyGraphChanged();
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UK2Node_Blueprint_Template::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);
    check(GetWorldContextPin());
    HideClassPin();

    UK2Node::AllocateDefaultPins();
    GetGraph()->NotifyGraphChanged();
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}


void UK2Node_Blueprint_Template::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    for (TObjectIterator<UClass> It; It; ++It)
    {
        UClass* TargetClass = *It;
        if (TargetClass->IsChildOf(ProxyFactoryClass))
        {
            RegisterBlueprintAction(TargetClass, ActionRegistrar);
        }
    }
}

void UK2Node_Blueprint_Template::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);
}

FText UK2Node_Blueprint_Template::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (ProxyClass)
    {
//++CK
        if (const auto* TargetClassAsBlueprintTask = Cast<UBlueprintTaskTemplate>(ProxyClass->ClassDefaultObject);
            IsValid(TargetClassAsBlueprintTask) && ProxyClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
        {
            if (const auto& MenuDisplayName = TargetClassAsBlueprintTask->MenuDisplayName;
                MenuDisplayName != NAME_None)
            { return FText::FromName(MenuDisplayName); }
        }
//--CK
        const FString Str = ProxyClass->GetName();
        TArray<FString> ParseNames;
        Str.ParseIntoArray(ParseNames, *FNames_Helper::CompiledFromBlueprintSuffix);
        return FText::FromString(ParseNames[0]);
    }
    return FText(LOCTEXT("UK2Node_Blueprint_Template", "Node_Blueprint_Template Function"));
}

FText UK2Node_Blueprint_Template::GetMenuCategory() const
{
    if (ProxyClass && ProxyClass->HasMetaData(TEXT("Category")))
    {
        return FText::FromString(ProxyClass->GetMetaData(TEXT("Category")));
    }
    return Super::GetMenuCategory();
}

//++CK
auto
    UK2Node_Blueprint_Template::
    CreateVisualWidget()
    -> TSharedPtr<SGraphNode>
{
    return SNew(SGraphNode_Blueprint_Template, this);
}

auto
    UK2Node_Blueprint_Template::
    Get_NodeDescription() const
    -> FString
{
    if (IsValid(GEditor->PlayWorld) == false && IsValid(ProxyClass))
    {
        if (const auto& TaskCDO = Cast<UBlueprintTaskTemplate>(ProxyClass->ClassDefaultObject);
            IsValid(TaskCDO))
        {
            const auto& NodeDescription = TaskCDO->Get_NodeDescription();
            return NodeDescription;
        }

        return {};
    }

    // TODO: Expose through developer settings
    if (constexpr auto ShowNodeDescriptionWhilePlaying = false;
        ShowNodeDescriptionWhilePlaying == false)
    { return {}; }

    if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBlueprintTask_EngineSubsystem_UE>();
        IsValid(Subsystem))
    {
        if (const auto FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
            IsValid(FoundTaskInstance))
        {
            const auto& NodeDescription = FoundTaskInstance->Get_NodeDescription();
            return NodeDescription;
        }
    }

    return {};
}

auto
    UK2Node_Blueprint_Template::
    Get_StatusString() const
    -> FString
{
    if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBlueprintTask_EngineSubsystem_UE>();
        IsValid(Subsystem))
    {
        if (const auto FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
            IsValid(FoundTaskInstance))
        {
            if (FoundTaskInstance->Get_IsActive() == false)
            { return {}; }

            const auto& NodeStatus = FoundTaskInstance->Get_StatusString();
            return NodeStatus;
        }
    }

    return {};
}

auto
    UK2Node_Blueprint_Template::
    Get_StatusBackgroundColor() const
    -> FLinearColor
{
    if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBlueprintTask_EngineSubsystem_UE>();
        IsValid(Subsystem))
    {
        if (const auto FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
            IsValid(FoundTaskInstance))
        {
            if (FLinearColor ObtainedColor;
                FoundTaskInstance->Get_StatusBackgroundColor(ObtainedColor))
            {
                return ObtainedColor;
            }
        }
    }

    // TODO: Expose through developer settings
    constexpr auto NodeStatusBackground = FLinearColor(0.12f, 0.12f, 0.12f, 1.0f);
    return NodeStatusBackground;
}

auto
    UK2Node_Blueprint_Template::
    Get_NodeConfigText() const
    -> FText
{
    if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBlueprintTask_EngineSubsystem_UE>();
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

auto
    UK2Node_Blueprint_Template::
    CustomizeProxyFactoryFunction(
        FKismetCompilerContext& CompilerContext,
        UEdGraph* SourceGraph,
        UK2Node_CallFunction* InCreateProxyObjecNode)
    -> void
{
    if (IsValid(InCreateProxyObjecNode) == false)
    { return; }

    // HACK: Associates the runtime node instance with the K2Node that created it.
    // This is required to enable customizations in Slate, as we use runtime data
    // from the node instance to dynamically populate information such as StatusString, Description, and more.
    if (const auto Pin = InCreateProxyObjecNode->FindPin(TEXT("NodeGuidStr"));
        Pin != nullptr)
    {
        if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
        {
            Pin->DefaultValue = NodeGuid.ToString();
        }
    }
}

auto
    UK2Node_Blueprint_Template::
    Get_PinsHiddenByDefault()
    -> TSet<FName>
{
    auto PinsHiddenByDefault = TSet{Super::Get_PinsHiddenByDefault()};
    PinsHiddenByDefault.Add(TEXT("NodeGuidStr"));

    return PinsHiddenByDefault;
}
//--CK

#if WITH_EDITORONLY_DATA
void UK2Node_Blueprint_Template::ResetToDefaultExposeOptions_Impl()
{
    if (ProxyClass)
    {
        if (const auto CDO = ProxyClass->GetDefaultObject<UBlueprintTaskTemplate>())
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

void UK2Node_Blueprint_Template::CollectSpawnParam(UClass* InClass, const bool bFullRefresh)
{
    if (InClass)
    {
        if (const UBlueprintTaskTemplate* CDO = InClass->GetDefaultObject<UBlueprintTaskTemplate>())
        {
            AllDelegates = CDO->AllDelegates;
            AllFunctions = CDO->AllFunctions;
            AllFunctionsExec = CDO->AllFunctionsExec;
            AllParam = CDO->AllParam;
            if (bFullRefresh)
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

//++CK
auto
    SGraphNode_Blueprint_Template::
    Construct(
        const FArguments& InArgs,
        UK2Node_Blueprint_Template* InNode)
    -> void
{
    GraphNode = InNode;
    _BlueprintTaskNode = InNode;

    SetCursor(EMouseCursor::CardinalCross);
    UpdateGraphNode();
}

auto
    SGraphNode_Blueprint_Template::
    GetNodeInfoPopups(
        FNodeInfoContext* Context,
        TArray<FGraphInformationPopupInfo>& Popups) const
    -> void
{
    if (const auto& Description = _BlueprintTaskNode->Get_NodeDescription();
        Description.IsEmpty() == false)
    {
        // TODO: Expose through developer settings
        constexpr auto NodeDescriptionBackGroundColor = FLinearColor(0.0625f, 0.0625f, 0.0625f, 1.0f);

        const auto DescriptionPopup = FGraphInformationPopupInfo(nullptr, NodeDescriptionBackGroundColor, Description);
        Popups.Add(DescriptionPopup);
    }

    if (GEditor->PlayWorld)
    {
        if (const auto& Status = _BlueprintTaskNode->Get_StatusString();
            Status.IsEmpty() == false)
        {
            const auto DescriptionPopup = FGraphInformationPopupInfo(nullptr, _BlueprintTaskNode->Get_StatusBackgroundColor(), Status);
            Popups.Add(DescriptionPopup);
        }
    }
}

auto
    SGraphNode_Blueprint_Template::
    CreateBelowPinControls(
        TSharedPtr<SVerticalBox> MainBox)
    -> void
{
    static const FMargin ConfigBoxPadding = FMargin(2.0f, 0.0f, 1.0f, 0.0);

    // Add a box to wrap around the Config Text area to make it a more visually distinct part of the node
    TSharedPtr<SVerticalBox> BelowPinsBox;
    MainBox->AddSlot()
        .AutoHeight()
        .Padding(ConfigBoxPadding)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
            .BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 1.0f))
            .Visibility(this, &SGraphNode_Blueprint_Template::Get_NodeConfigTextVisibility)
            [
                SAssignNew(BelowPinsBox, SVerticalBox)
            ]
        ];

    static const FMargin ConfigTextPadding = FMargin(2.0f, 0.0f, 0.0f, 3.0f);

    BelowPinsBox->AddSlot()
        .AutoHeight()
        .Padding(ConfigTextPadding)
        [
            SAssignNew(_ConfigTextBlock, STextBlock)
            .AutoWrapText(true)
            .LineBreakPolicy(FBreakIterator::CreateWordBreakIterator())
            .Text(this, &SGraphNode_Blueprint_Template::Get_NodeConfigText)
        ];
}

auto
    SGraphNode_Blueprint_Template::
    Get_NodeConfigTextVisibility() const
    -> EVisibility
{
    // Hide in lower LODs
    if (const TSharedPtr<SGraphPanel> OwnerPanel = GetOwnerPanel();
        OwnerPanel.IsValid() == false || OwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::MediumDetail)
    {
        if (_ConfigTextBlock.IsValid() && _ConfigTextBlock->GetText().IsEmptyOrWhitespace() == false)
        {
            return EVisibility::Visible;
        }
    }

    return EVisibility::Collapsed;
}

auto
    SGraphNode_Blueprint_Template::
    Get_NodeConfigText() const
    -> FText
{
    if (IsValid(_BlueprintTaskNode.Get()))
    { return _BlueprintTaskNode->Get_NodeConfigText(); }

    return FText::GetEmpty();
}

//-CK

#undef LOCTEXT_NAMESPACE