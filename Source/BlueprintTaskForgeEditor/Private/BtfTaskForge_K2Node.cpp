#include "BtfTaskForge_K2Node.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintTaskForge/Public/BtfTaskForge.h"
#include "BlueprintTaskForge/Public/Subsystem/BtfSubsystem.h"

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
    if (Decorator.IsValid())
    {
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

    const auto HasInvalidFlags = TargetClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated);
    const auto HasInvalidName = (Name.Contains(FNames_Helper::SkelPrefix) || 
                                Name.Contains(FNames_Helper::ReinstPrefix) || 
                                Name.Contains(FNames_Helper::DeadclassPrefix));

    if (HasInvalidFlags || HasInvalidName)
    { return; }

    auto* NodeClass = GetClass();
    auto Lambda = [NodeClass, TargetClass](const UFunction* FactoryFunc) -> UBlueprintNodeSpawner*
    {
        auto CustomizeTimelineNodeLambda = [TargetClass](UEdGraphNode* NewNode, bool IsTemplateNode, const TWeakObjectPtr<UFunction> FunctionPtr)
        {
            auto* AsyncTaskNode = CastChecked<UBtf_TaskForge_K2Node>(NewNode);
            if (NOT FunctionPtr.IsValid())
            { return; }

            auto* Func = FunctionPtr.Get();
            AsyncTaskNode->ProxyFactoryFunctionName = Func->GetFName();
            AsyncTaskNode->ProxyClass = TargetClass;
        };

        auto* NodeSpawner = UBlueprintFunctionNodeSpawner::Create(FactoryFunc);
        NodeSpawner->NodeClass = NodeClass;
        
        if (TargetClass && TargetClass->HasMetaData(TEXT("Category")))
        {
            NodeSpawner->DefaultMenuSignature.Category = FText::FromString(TargetClass->GetMetaData(TEXT("Category")));
        }

        const auto FunctionPtr = MakeWeakObjectPtr(const_cast<UFunction*>(FactoryFunc));
        auto& MenuSignature = NodeSpawner->DefaultMenuSignature;

        auto LocName = TargetClass->GetName();
        LocName.RemoveFromEnd(FNames_Helper::CompiledFromBlueprintSuffix);

        if (const auto* TargetClassAsBlueprintTask = Cast<UBtf_TaskForge>(TargetClass->ClassDefaultObject);
            IsValid(TargetClassAsBlueprintTask))
        {
            if (TargetClassAsBlueprintTask->Get_Category() != NAME_None)
            {
                MenuSignature.Category = FText::FromName(TargetClassAsBlueprintTask->Get_Category());
            }

            if (TargetClassAsBlueprintTask->Get_Tooltip() != NAME_None)
            {
                MenuSignature.Tooltip = FText::FromName(TargetClassAsBlueprintTask->Get_Tooltip());
            }

            MenuSignature.MenuName = TargetClassAsBlueprintTask->Get_MenuDisplayName() != NAME_None
                                        ? FText::FromName(TargetClassAsBlueprintTask->Get_MenuDisplayName())
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

    for (TFieldIterator<UFunction> FuncIt(TargetClass); FuncIt; ++FuncIt)
    {
        const auto* Function = *FuncIt;
        if (NOT Function->HasAnyFunctionFlags(FUNC_Static))
        { continue; }
        
        if (CastField<FObjectProperty>(Function->GetReturnProperty()) == nullptr)
        { continue; }

        if (auto* NewAction = Lambda(Function); 
            IsValid(NewAction))
        {
            ActionRegistrar.AddBlueprintAction(Function, NewAction);
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
    if (NOT ProxyClass)
    { return FText(LOCTEXT("UBtf_TaskForge_K2Node", "Node_Blueprint_Task Function")); }

    if (const auto* TargetClassAsBlueprintTask = Cast<UBtf_TaskForge>(ProxyClass->ClassDefaultObject);
        IsValid(TargetClassAsBlueprintTask))
    {
        const auto& MenuDisplayName = TargetClassAsBlueprintTask->Get_MenuDisplayName();
        if (MenuDisplayName != NAME_None)
        {
            return FText::FromName(MenuDisplayName);
        }
    }

    const auto Str = ProxyClass->GetName();
    auto ParseNames = TArray<FString>{};
    Str.ParseIntoArray(ParseNames, *FNames_Helper::CompiledFromBlueprintSuffix);
    return FText::FromString(ParseNames[0]);
}

FText UBtf_TaskForge_K2Node::GetMenuCategory() const
{
    if (ProxyClass && ProxyClass->HasMetaData(TEXT("Category")))
    {
        return FText::FromString(ProxyClass->GetMetaData(TEXT("Category")));
    }
    return Super::GetMenuCategory();
}

FString UBtf_TaskForge_K2Node::Get_NodeDescription() const
{
    if (NOT IsValid(GEditor->PlayWorld) && IsValid(ProxyClass))
    {
        if (TaskInstance)
        {
            return TaskInstance->Get_NodeDescription_Implementation();
        }

        if (const auto* TaskCDO = Cast<UBtf_TaskForge>(ProxyClass->ClassDefaultObject);
            IsValid(TaskCDO))
        {
            return TaskCDO->Get_NodeDescription_Implementation();
        }

        return {};
    }

    // TODO: Expose through developer settings
    constexpr auto ShowNodeDescriptionWhilePlaying = false;
    if (NOT ShowNodeDescriptionWhilePlaying)
    { return {}; }

    if (const auto* Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
        IsValid(Subsystem))
    {
        if (const auto* FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
            IsValid(FoundTaskInstance))
        {
            return FoundTaskInstance->Get_NodeDescription_Implementation();
        }
    }

    return {};
}

FString UBtf_TaskForge_K2Node::Get_StatusString() const
{
    const auto* Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
    if (NOT IsValid(Subsystem))
    { return {}; }

    const auto* FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
    if (NOT IsValid(FoundTaskInstance))
    { return {}; }
    
    if (NOT FoundTaskInstance->Get_IsActive())
    { return {}; }

    return FoundTaskInstance->Get_StatusString_Implementation();
}

FLinearColor UBtf_TaskForge_K2Node::Get_StatusBackgroundColor() const
{
    // TODO: Expose through developer settings
    constexpr auto NodeStatusBackground = FLinearColor(0.12f, 0.12f, 0.12f, 1.0f);

    const auto* Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
    if (NOT IsValid(Subsystem))
    { return NodeStatusBackground; }

    const auto* FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
    if (NOT IsValid(FoundTaskInstance))
    { return NodeStatusBackground; }

    auto ObtainedColor = FLinearColor{};
    if (FoundTaskInstance->Get_StatusBackgroundColor_Implementation(ObtainedColor))
    {
        return ObtainedColor;
    }

    return NodeStatusBackground;
}

FText UBtf_TaskForge_K2Node::Get_NodeConfigText() const
{
    const auto* Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
    if (NOT IsValid(Subsystem))
    { return FText::GetEmpty(); }

    const auto* FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
    if (NOT IsValid(FoundTaskInstance))
    { return FText::GetEmpty(); }

    if (FoundTaskInstance->Get_IsActive())
    {
        return FText::FromName(TEXT("Running..."));
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
    if (NOT ProxyClass)
    {
        ReconstructNode();
        return;
    }

    const auto* CDO = ProxyClass->GetDefaultObject<UBtf_TaskForge>();
    if (NOT IsValid(CDO))
    {
        ReconstructNode();
        return;
    }

    AllDelegates = CDO->Get_AllDelegates();
    AllFunctions = CDO->Get_AllFunctions();
    AllFunctionsExec = CDO->Get_AllFunctionsExec();
    AllParam = CDO->Get_AllParam();

    SpawnParam = CDO->Get_SpawnParam();
    AutoCallFunction = CDO->Get_AutoCallFunction();
    ExecFunction = CDO->Get_ExecFunction();
    InDelegate = CDO->Get_InDelegate();
    OutDelegate = CDO->Get_OutDelegate();

    ReconstructNode();
}
#endif

void UBtf_TaskForge_K2Node::CollectSpawnParam(UClass* InClass, const bool FullRefresh)
{
    if (NOT InClass)
    { return; }

    const auto* CDO = InClass->GetDefaultObject<UBtf_TaskForge>();
    if (NOT IsValid(CDO))
    { return; }

    AllDelegates = CDO->Get_AllDelegates();
    AllFunctions = CDO->Get_AllFunctions();
    AllFunctionsExec = CDO->Get_AllFunctionsExec();
    AllParam = CDO->Get_AllParam();
    
    if (NOT FullRefresh)
    { return; }

    SpawnParam = CDO->Get_SpawnParam();
    AutoCallFunction = CDO->Get_AutoCallFunction();
    ExecFunction = CDO->Get_ExecFunction();
    InDelegate = CDO->Get_InDelegate();
    OutDelegate = CDO->Get_OutDelegate();
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
