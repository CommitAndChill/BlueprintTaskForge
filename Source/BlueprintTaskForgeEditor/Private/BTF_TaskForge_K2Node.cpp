#include "Btf_TaskForge_K2Node.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include "BlueprintTaskForge/Public/Btf_TaskForge.h"
#include "BlueprintTaskForge/Public/Subsystem/Btf_Subsystem.h"

#define LOCTEXT_NAMESPACE "K2Node"

UBtf_TaskForge_K2Node::UBtf_TaskForge_K2Node(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UBtf_TaskForge, BlueprintTaskTemplate);
    ProxyFactoryClass = UBtf_TaskForge::StaticClass();
    OutPutObjectPinName = FName(TEXT("TaskObject"));
    //AutoCallFunctions.Add(GET_FUNCTION_NAME_CHECKED(UBtf_TaskForge, Init_Activate));
}

void UBtf_TaskForge_K2Node::HideClassPin() const
{
    UEdGraphPin* ClassPin = FindPinChecked(ClassPinName);
    ClassPin->DefaultObject = ProxyClass;
    ClassPin->DefaultValue.Empty();
    ClassPin->bDefaultValueIsReadOnly = true;
    ClassPin->bNotConnectable = true;
    ClassPin->bHidden = true;
}

void UBtf_TaskForge_K2Node::RegisterBlueprintAction(UClass* TargetClass, FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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
                UBtf_TaskForge_K2Node* AsyncTaskNode = CastChecked<UBtf_TaskForge_K2Node>(NewNode);
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

                if (const UBtf_TaskForge* TargetClassAsBlueprintTask = Cast<UBtf_TaskForge>(TargetClass->ClassDefaultObject))
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

void UBtf_TaskForge_K2Node::AllocateDefaultPins()
{
    Super::AllocateDefaultPins();
    check(GetWorldContextPin());
    HideClassPin();

	FindPin(NodeGuidStrName)->DefaultValue = ProxyClass->GetName() + NodeGuid.ToString();
	FindPin(NodeGuidStrName)->bHidden = true; //Why isn't this working?
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
        UClass* TargetClass = *It;
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
    if (ProxyClass)
    {
//++CK
		if (const auto* TargetClassAsBlueprintTask = Cast<UBtf_TaskForge>(ProxyClass->ClassDefaultObject);
			IsValid(TargetClassAsBlueprintTask))
		{
			const auto& MenuDisplayName = TargetClassAsBlueprintTask->MenuDisplayName;
		    if (MenuDisplayName != NAME_None)
			{ return FText::FromName(MenuDisplayName); }
		}
//--CK
        const FString Str = ProxyClass->GetName();
        TArray<FString> ParseNames;
        Str.ParseIntoArray(ParseNames, *FNames_Helper::CompiledFromBlueprintSuffix);
        return FText::FromString(ParseNames[0]);
    }
    return FText(LOCTEXT("UBtf_TaskForge_K2Node", "Node_Blueprint_Template Function"));
}

FText UBtf_TaskForge_K2Node::GetMenuCategory() const
{
    if (ProxyClass && ProxyClass->HasMetaData(TEXT("Category")))
    {
        return FText::FromString(ProxyClass->GetMetaData(TEXT("Category")));
    }
    return Super::GetMenuCategory();
}

auto
    UBtf_TaskForge_K2Node::
    Get_NodeDescription() const
    -> FString
{
    if (IsValid(GEditor->PlayWorld) == false && IsValid(ProxyClass))
    {
    	if(TaskInstance)
    	{
    		return TaskInstance->Get_NodeDescription_Implementation();
    	}

        if (const auto& TaskCDO = Cast<UBtf_TaskForge>(ProxyClass->ClassDefaultObject);
            IsValid(TaskCDO))
        {
            const auto& NodeDescription = TaskCDO->Get_NodeDescription_Implementation();
            return NodeDescription;
        }

        return {};
    }

    // TODO: Expose through developer settings
    if (constexpr auto ShowNodeDescriptionWhilePlaying = false;
        ShowNodeDescriptionWhilePlaying == false)
    { return {}; }

    if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
        IsValid(Subsystem))
    {
        if (const auto FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
            IsValid(FoundTaskInstance))
        {
            const auto& NodeDescription = FoundTaskInstance->Get_NodeDescription_Implementation();
            return NodeDescription;
        }
    }

    return {};
}

auto
    UBtf_TaskForge_K2Node::
    Get_StatusString() const
    -> FString
{
    if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
        IsValid(Subsystem))
    {
        if (const auto FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
            IsValid(FoundTaskInstance))
        {
            if (FoundTaskInstance->Get_IsActive() == false)
            { return {}; }

            const auto& NodeStatus = FoundTaskInstance->Get_StatusString_Implementation();
            return NodeStatus;
        }
    }

    return {};
}

auto
    UBtf_TaskForge_K2Node::
    Get_StatusBackgroundColor() const
    -> FLinearColor
{
    if (const auto& Subsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
        IsValid(Subsystem))
    {
        if (const auto FoundTaskInstance = Subsystem->FindTaskInstanceWithGuid(NodeGuid);
            IsValid(FoundTaskInstance))
        {
            if (FLinearColor ObtainedColor;
                FoundTaskInstance->Get_StatusBackgroundColor_Implementation(ObtainedColor))
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
    UBtf_TaskForge_K2Node::
    Get_NodeConfigText() const
    -> FText
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

auto
    UBtf_TaskForge_K2Node::
    Get_PinsHiddenByDefault()
    -> TSet<FName>
{
    auto PinsHiddenByDefault = TSet{Super::Get_PinsHiddenByDefault()};
    PinsHiddenByDefault.Add(TEXT("NodeGuidStr"));

    return PinsHiddenByDefault;
}
//--CK

#if WITH_EDITORONLY_DATA
void UBtf_TaskForge_K2Node::ResetToDefaultExposeOptions_Impl()
{
    if (ProxyClass)
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

void UBtf_TaskForge_K2Node::CollectSpawnParam(UClass* InClass, const bool bFullRefresh)
{
    if (InClass)
    {
        if (const UBtf_TaskForge* CDO = InClass->GetDefaultObject<UBtf_TaskForge>())
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
#undef LOCTEXT_NAMESPACE