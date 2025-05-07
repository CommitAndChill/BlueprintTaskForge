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
#include "Framework/Commands/UIAction.h"
#include "ToolMenu.h"
#include "K2Node_SwitchName.h"
#include "ObjectTools.h"

#include "NodeCustomizations/BtfGraphNode.h"

#include "StructUtils/InstancedStruct.h"


#define LOCTEXT_NAMESPACE "K2Node"


// clang-format off
FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::InPrefix			= FString::Printf(TEXT("In_"));
FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::OutPrefix			= FString::Printf(TEXT("Out_"));
FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::InitPrefix		= FString::Printf(TEXT("Init_"));

FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::SkelPrefix		= FString::Printf(TEXT("SKEL_"));
FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::ReinstPrefix		= FString::Printf(TEXT("REINST_"));
FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::DeadclassPrefix	= FString::Printf(TEXT("DEADCLASS_"));

FString UBtf_ExtendConstructObject_K2Node::FNames_Helper::CompiledFromBlueprintSuffix = FString::Printf(TEXT("_C"));
// clang-format on


UBtf_ExtendConstructObject_K2Node::UBtf_ExtendConstructObject_K2Node(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , ProxyFactoryClass(nullptr)
    , ProxyClass(nullptr)
    , ProxyFactoryFunctionName(NAME_None)
    , bPinTooltipsValid(false)
{
    ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UBtf_ExtendConstructObject_Utils, ExtendConstructObject);
    ProxyFactoryClass = UBtf_ExtendConstructObject_Utils::StaticClass();
    WorldContextPinName = FName(TEXT("Outer"));
}

UEdGraphPin* UBtf_ExtendConstructObject_K2Node::GetWorldContextPin() const
{
    if (bSelfContext)
    {
        return FindPin(UEdGraphSchema_K2::PSC_Self);
    }
    return FindPin(WorldContextPinName); //old}
}

#if WITH_EDITORONLY_DATA
void UBtf_ExtendConstructObject_K2Node::PostEditChangeProperty(FPropertyChangedEvent& e)
{
    bool bNeedReconstruct = false;
    if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, AutoCallFunction))
    {
        RefreshNames(ExecFunction, false);
        for (auto& It : AutoCallFunction)
        {
            It.SetAllExclude(AllFunctions, AutoCallFunction);
            ExecFunction.Remove(It);
        }
        bNeedReconstruct = true;
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, ExecFunction))
    {
        RefreshNames(ExecFunction, false);
        for (auto& It : ExecFunction)
        {
            It.SetAllExclude(AllFunctionsExec, ExecFunction);
            AutoCallFunction.Remove(It);
        }
        bNeedReconstruct = true;
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, InDelegate))
    {
        RefreshNames(InDelegate, false);
        for (auto& It : InDelegate)
        {
            It.SetAllExclude(AllDelegates, InDelegate);
            OutDelegate.Remove(It);
        }
        bNeedReconstruct = true;
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, OutDelegate))
    {
        RefreshNames(OutDelegate, false);
        for (auto& It : OutDelegate)
        {
            It.SetAllExclude(AllDelegates, OutDelegate);
            InDelegate.Remove(It);
        }
        bNeedReconstruct = true;
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, SpawnParam))
    {
        RefreshNames(SpawnParam, false);
        for (auto& It : SpawnParam)
        {
            It.SetAllExclude(AllParam, SpawnParam);
        }
        bNeedReconstruct = true;
    }
    else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, bOwnerContextPin))
    {
        bNeedReconstruct = true;

		if (bOwnerContextPin)
		{
			bSelfContext = false;
		}
	}
	else if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_ExtendConstructObject_K2Node, AllowInstance))
	{
		bNeedReconstruct = true;

		if(AllowInstance)
		{
			/**Double check to see if an extension with our GUID exists.
			 * This is mostly just to cover very niche scenarios where the extension
			 * is not removed properly. */
			bool InstanceFound = false;
			for(auto& CurrentExtension : GetBlueprint()->GetExtensions())
			{
				if(CurrentExtension.IsA(UBtf_TaskForge::StaticClass()) && CurrentExtension.GetName().Contains(NodeGuid.ToString()))
				{
					TaskInstance = Cast<UBtf_TaskForge>(CurrentExtension);
					TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent PropertyChangedEvent)
					{
						ReconstructNode();
					});
					InstanceFound = true;
				}
			}

			if(!InstanceFound)
			{
				/**IMPORTANT: The outer seems to be important. If it wasn't the parent blueprint, it would always
				 * remove the extension during packaging/standalone mode.
				 * If you're getting weird issues with the extension getting "lost", then this might be the
				 * first place to investigate. */
				TaskInstance = NewObject<UBtf_TaskForge>(GetBlueprint(), ProxyClass, GetTemplateInstanceName());
				GetBlueprint()->AddExtension(TaskInstance);
				std::ignore = GetBlueprint()->MarkPackageDirty();
				std::ignore = TaskInstance->MarkPackageDirty();

				// TaskInstance->OnPostPropertyChanged.BindSP(this, &UBtf_ExtendConstructObject_K2Node::OnTaskPropertyChanged);
				TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent PropertyChangedEvent)
				{
					ReconstructNode();
				});
			}
		}
		else
		{
			if(TaskInstance)
			{
				TaskInstance->OnPostPropertyChanged.Unbind();
				GetBlueprint()->RemoveExtension(TaskInstance);
				TaskInstance->MarkAsGarbage();
				TaskInstance = nullptr;
			}
		}

		/**Refresh the details panel, so we can instantly modify the instance*/
		if(DetailsPanelBuilder)
		{
			DetailsPanelBuilder->ForceRefreshDetails();
		}
	}

    if (bNeedReconstruct)
    {
        Modify();
        ReconstructNode();
        //todo: Warning: you need to make sure that the reconstruction of the node is not called when copying it
    }

	bool bIsDirty = false;
	FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == TEXT("CustomPins"))
	{
		bIsDirty = true;
	}

	if (bIsDirty)
	{
		ReconstructNode();
	}

	Super::PostEditChangeProperty(e);
}
void UBtf_ExtendConstructObject_K2Node::ResetToDefaultExposeOptions_Impl()
{
    //if (ProxyClass)
    //{
    //	if (const UObject* CDO = ProxyClass->GetDefaultObject<UObject>())
    //	{}
    //}
    ReconstructNode();
}
#endif


void UBtf_ExtendConstructObject_K2Node::Serialize(FArchive& Ar)
{
    Ar.UsingCustomVersion(FFrameworkObjectVersion::GUID);

    if (Ar.IsSaving())
    {
        if (ProxyClass && !ProxyClass->IsChildOf(UObject::StaticClass()))
        {
            ProxyClass = nullptr;
        }
    }

    Super::Serialize(Ar);

    if (Ar.IsLoading() /*&& ((Ar.GetPortFlags() & PPF_Duplicate) == 0)*/)
    {
        if (ProxyClass && !ProxyClass->IsChildOf(UObject::StaticClass()))
        {
            ProxyClass = nullptr;
        }
        if (!bOwnerContextPin && FindPin(WorldContextPinName))
        {
            bSelfContext = false;
            bOwnerContextPin = true;
        }
    }
}


void UBtf_ExtendConstructObject_K2Node::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
    Super::GetNodeContextMenuActions(Menu, Context);
    if (Context->bIsDebugging) return;

    if (Context->Node != nullptr && Context->Node->GetClass()->IsChildOf(UBtf_ExtendConstructObject_K2Node::StaticClass()) && Context->Pin == nullptr)
    {
        FToolMenuSection& Section = Menu->AddSection("ExtendNode", LOCTEXT("TemplateNode", "TemplateNode"));

        if (AllParam.Num())
        {
            Section.AddSubMenu(
                FName("Add_SpawnParamPin"),
                FText::FromName(FName("AddSpawnParamPin")),
                FText(),
                FNewToolMenuDelegate::CreateLambda(
                    [&](UToolMenu* InMenu)
                    {
                        FToolMenuSection& SubMenuSection = InMenu->AddSection("AddSpawnParamPin", FText::FromName(FName("AddSpawnParamPin")));
                        for (const FName& It : AllParam)
                        {
                            if (!SpawnParam.Contains(It))
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
                        FToolMenuSection& SubMenuSection = InMenu->AddSection("AddAutoCallFunction", LOCTEXT("AddAutoCallFunction", "AddAutoCallFunction"));
                        for (const FName& It : AllFunctions)
                        {
                            if (!AutoCallFunction.Contains(It))
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
                        FToolMenuSection& SubMenuSection = InMenu->AddSection("AddExecCallFunction", LOCTEXT("AddExecCallFunction", "AddCallFunctionPin"));
                        for (const FName& It : AllFunctionsExec)
                        {
                            if (!ExecFunction.Contains(It))
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
                        FToolMenuSection& SubMenuSection = InMenu->AddSection("AddInputDelegatePin", FText::FromName(FName("AddInputDelegatePin")));
                        for (const FName& It : AllDelegates)
                        {
                            if (!InDelegate.Contains(It))
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
                        FToolMenuSection& SubMenuSection = InMenu->AddSection("SubAsyncTaskSubMenu", FText::FromName(FName("AddOutputDelegatePin")));
                        for (const FName& It : AllDelegates)
                        {
                            if (!OutDelegate.Contains(It))
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

    if (Context->Node != nullptr &&															  //
        Context->Node->GetClass()->IsChildOf(UBtf_ExtendConstructObject_K2Node::StaticClass()) && //
        Context->Pin != nullptr)
    {
        if (Context->Pin->PinName == UEdGraphSchema_K2::PN_Execute || Context->Pin->PinName == UEdGraphSchema_K2::PN_Then) return;

        const bool bSpawnParamPin = SpawnParam.Contains(Context->Pin->PinName);

        if (Context->Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec || Context->Pin->PinType.PinCategory == UEdGraphSchema_K2::PN_EntryPoint ||
            (Context->Pin->Direction == EGPD_Input && Context->Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Delegate) || bSpawnParamPin)
        {
            const FName RemPinName = Context->Pin->PinName;
            if (bSpawnParamPin ||						 //
                ExecFunction.Contains(RemPinName) ||	 //
                AutoCallFunction.Contains(RemPinName) || //
                InDelegate.Contains(RemPinName) ||		 //
                OutDelegate.Contains(RemPinName))
            {
                FToolMenuSection& Section = Menu->AddSection("ExtendNode", LOCTEXT("ExtendNode", "ExtendNode"));
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
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);

        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}


FText UBtf_ExtendConstructObject_K2Node::GetMenuCategory() const
{
    if (const UFunction* TargetFunction = GetFactoryFunction())
    {
        return UK2Node_CallFunction::GetDefaultCategoryForFunction(TargetFunction, FText::GetEmpty());
    }
    return FText();
}

bool UBtf_ExtendConstructObject_K2Node::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
    const EGraphType GraphType = TargetGraph->GetSchema()->GetGraphType(TargetGraph);
    const bool bIsCompatible = GraphType == EGraphType::GT_Ubergraph || GraphType == EGraphType::GT_Macro || GraphType == EGraphType::GT_Function;
    return bIsCompatible && Super::IsCompatibleWithGraph(TargetGraph);
}

void UBtf_ExtendConstructObject_K2Node::GetMenuEntries(struct FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    Super::GetMenuEntries(ContextMenuBuilder);
}

void UBtf_ExtendConstructObject_K2Node::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
    Super::ValidateNodeDuringCompilation(MessageLog);

    if (UObject const* SourceObject = MessageLog.FindSourceObject(this))
    {
        // Lets check if it's a result of macro expansion, to give a helpful error
        if (UK2Node_MacroInstance const* MacroInstance = Cast<UK2Node_MacroInstance>(SourceObject))
        {
            // Since it's not possible to check the graph's type, just check if this is a ubergraph using the schema's name for it
            if (!(GetGraph()->HasAnyFlags(RF_Transient) && GetGraph()->GetName().StartsWith(UEdGraphSchema_K2::FN_ExecuteUbergraphBase.ToString())))
            {
                MessageLog.Error(
                    *LOCTEXT("AsyncTaskInFunctionFromMacro", "@@ is being used in Function '@@' resulting from expansion of Macro '@@'").ToString(),
                    this,
                    GetGraph(),
                    MacroInstance);
            }
        }
    }

	if (ProxyClass == nullptr)
	{
		if (UObject const* SourceObject = MessageLog.FindSourceObject(this))
		{
			if (UK2Node_MacroInstance const* MacroInstance = Cast<UK2Node_MacroInstance>(SourceObject))
			{
				MessageLog.Error(
					*LOCTEXT("ExtendConstructObject", "@@ is being used in Function '@@' resulting from expansion of Macro '@@'").ToString(),
					this,
					GetGraph(),
					MacroInstance);
			}
		}
	}
	const UEdGraphPin* ClassPin = FindPin(ClassPinName, EGPD_Input);
	if (!(ProxyClass && ClassPin && ClassPin->Direction == EGPD_Input))
	{
		MessageLog.Error(
			*FText::Format(
				 LOCTEXT("GenericCreateObject_WrongClassFmt", "Cannot construct Node Class of type '{0}' in @@"),
				 FText::FromString(GetPathNameSafe(ProxyClass)))
				 .ToString(),
			this);
	}

	if(!CanBePlacedInGraph())
	{
		MessageLog.Error(
			*FText::Format(
				 LOCTEXT("ExtendConstructObjectPlacedInGraph", "{0} is not allowed to be placed in this class. @@"),
				 FText::FromString(GetPathNameSafe(ProxyClass))).ToString(), this);
	}

	if(UBtf_TaskForge* Task = GetInstanceOrDefaultObject())
	{
		TArray<FString> Errors = Task->ValidateNodeDuringCompilation();
		for(const FString& It : Errors)
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
    if (!bPinTooltipsValid)
    {
        for (UEdGraphPin* P : Pins)
        {
            if (P->Direction == EGPD_Input)
            {
                P->PinToolTip.Reset();
                GeneratePinTooltip(*P);
            }
        }
        bPinTooltipsValid = true;
    }
    return Super::GetPinHoverText(Pin, HoverTextOut);
}

void UBtf_ExtendConstructObject_K2Node::EarlyValidation(class FCompilerResultsLog& MessageLog) const
{
    Super::EarlyValidation(MessageLog);

    if (ProxyClass == nullptr)
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

    UEdGraphSchema const* Schema = GetSchema();
    check(Schema);
    UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>(Schema);

    if (K2Schema == nullptr)
    {
        Schema->ConstructBasicPinTooltip(Pin, FText::GetEmpty(), Pin.PinToolTip);
        return;
    }

    // get the class function object associated with this node
    // Slight change from UK2Node_CallFunction (where this code is copied from)
    // We're getting the Factory function instead of GetTargetFunction
    const UFunction* Function = GetFactoryFunction();
    if (Function == nullptr)
    {
        Schema->ConstructBasicPinTooltip(Pin, FText::GetEmpty(), Pin.PinToolTip);
        return;
    }

    UK2Node_CallFunction::GeneratePinTooltipFromFunction(Pin, Function);
}

bool UBtf_ExtendConstructObject_K2Node::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const
{
    const UBlueprint* SourceBlueprint = GetBlueprint();

    const bool bProxyFactoryResult = (ProxyFactoryClass != nullptr) && (ProxyFactoryClass->ClassGeneratedBy != SourceBlueprint);
    if (bProxyFactoryResult && OptionalOutput)
    {
        OptionalOutput->AddUnique(ProxyFactoryClass);
    }

    const bool bProxyResult = (ProxyClass != nullptr) && (ProxyClass->ClassGeneratedBy != SourceBlueprint);
    if (bProxyResult && OptionalOutput)
    {
        OptionalOutput->AddUnique(ProxyClass);
    }

    const bool bSuperResult = Super::HasExternalDependencies(OptionalOutput);

    return bSuperResult || bProxyFactoryResult || bProxyResult;
}

FText UBtf_ExtendConstructObject_K2Node::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (ProxyClass)
    {
        FString Str = ProxyClass->GetName();
        Str.RemoveFromEnd(FNames_Helper::CompiledFromBlueprintSuffix, ESearchCase::Type::CaseSensitive);
        return FText::FromString(FString::Printf(TEXT("%s"), *Str));
    }
    return FText(LOCTEXT("UBtf_ExtendConstructObject_K2Node", "ExtendConstructObject"));
}

FText UBtf_ExtendConstructObject_K2Node::GetTooltipText() const
{
	const FString FunctionToolTipText = ObjectTools::GetDefaultTooltipForFunction(GetFactoryFunction());
	return FText::FromString(FunctionToolTipText);
}

FLinearColor UBtf_ExtendConstructObject_K2Node::GetNodeTitleColor() const
{
	FLinearColor CustomColor = FLinearColor();
	if(GetInstanceOrDefaultObject()->GetNodeTitleColor(CustomColor))
	{
		return CustomColor;
	}

	return Super::GetNodeTitleColor();
}

FName UBtf_ExtendConstructObject_K2Node::GetCornerIcon() const
{
    if (ProxyClass)
    {
        //if (const auto Btt = ProxyClass->GetDefaultObject())
        {
            if (OutDelegate.Num() == 0)
            {
                return FName();
            }
        }
    }
    return TEXT("Graph.Latent.LatentIcon");
}

bool UBtf_ExtendConstructObject_K2Node::UseWorldContext() const
{
    const UBlueprint* BP = GetBlueprint();
    const UClass* ParentClass = BP ? BP->ParentClass : nullptr;
    return ParentClass ? ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin) != nullptr : false;
}

FString UBtf_ExtendConstructObject_K2Node::GetPinMetaData(const FName InPinName, const FName InKey)
{
    //if (InPinName == ClassPinName && InKey == FBlueprintMetadata::MD_AllowAbstractClasses)
    //{
    //	if (NodeFixedClass)
    //	{
    //		return FString(TEXT("true"));
    //	}
    //	return FString(TEXT("false"));
    //}

//++CK
    if (const TMap<FName, FString>* Metadata = _PinMetadataMap.Find(InPinName))
    {
        if (const FString* Value = Metadata->Find(InKey))
        {
            return *Value;
        }
    }
//--CK

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
		//AutoCallFunction.Remove(It);
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
		//InDelegate.Remove(It);
	}

	/**TODO: When pasting a node, the TaskInstance will point to the old nodes task instance.
	 * But we need to generate a new one, BUT this node seemingly doesn't have a new GUID
	 * generated by this point. So we can't generate a new task instance.
	 * For now, just disable the instance and wipe the pointer*/
	AllowInstance = false;
	TaskInstance = nullptr;

	Super::PostPasteNode();
}

UObject* UBtf_ExtendConstructObject_K2Node::GetJumpTargetForDoubleClick() const
{
    if (ProxyClass)
    {
        return ProxyClass->ClassGeneratedBy;
    }
    return nullptr;
}

bool UBtf_ExtendConstructObject_K2Node::CanBePlacedInGraph() const
{
	/**Check if the node is limited to specific classes.
	 * If so, then return whether it's allowed in the current class. */
	if(UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(GetGraph()))
	{
		if(ProxyClass && Cast<UBtf_TaskForge>(ProxyClass->ClassDefaultObject) && Cast<UBtf_TaskForge>(ProxyClass->ClassDefaultObject)->ClassLimitations.IsValidIndex(0))
		{
			for(auto& CurrentClass : Cast<UBtf_TaskForge>(ProxyClass->ClassDefaultObject)->ClassLimitations)
			{
				if(!CurrentClass.IsNull())
				{
					if(CurrentClass->GetClassPathName().ToString() == Blueprint->GetPathName() + "_C") //Blueprints have _C appended to the name
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
	return AllowInstance && TaskInstance ? TaskInstance : ProxyClass->GetDefaultObject<UBtf_TaskForge>();
}

void UBtf_ExtendConstructObject_K2Node::GenerateCustomOutputPins()
{
	if(ProxyClass)
	{
		if(auto* TargetClassAsBlueprintTask = GetInstanceOrDefaultObject())
		{
			TArray<FCustomOutputPin> OldCustomPins = CustomPins;
			CustomPins = TargetClassAsBlueprintTask->GetCustomOutputPins();

			for(const auto& Pin : CustomPins)
			{
				if(!FindPin(Pin.PinName))
				{
					CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, FName(Pin.PinName))->PinToolTip = Pin.Tooltip;
				}
				else
				{
					//Pin already exists. Check if they should be removed
					for(auto& OldPin : OldCustomPins)
					{
						if(UEdGraphPin* OldPinRef =  FindPin(OldPin.PinName))
						{
							RemovePin(OldPinRef);
						}
					}
				}
			}

			//Custom pin data is only relevant if we have any custom pins
			if(CustomPins.IsValidIndex(0))
			{
				if(!FindPin(FName("Custom Pin Data")))
				{
					UEdGraphPin* DataPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, "Custom Pin Data");
					DataPin->PinType.PinSubCategoryObject = TBaseStructure<FInstancedStruct>::Get();
				}
			}
		}
	}
}

TSharedPtr<SGraphNode> UBtf_ExtendConstructObject_K2Node::CreateVisualWidget()
{
	if(TaskInstance)
	{
		/**V: I can't figure out how or where, but @TaskInstance is seemingly getting serialized (?)
		 * and I can't figure out what is supposed to be the "begin play" for this node, so I can't
		 * figure out what is the actual best spot to bind this delegate. Not even the constructor
		 * is triggered. */
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

	if(TaskInstance)
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
			const UEdGraphPin* SourcePin = Pin->LinkedTo[0];
			ProxyClass = SourcePin //
				? Cast<UClass>(SourcePin->PinType.PinSubCategoryObject.Get())
				: nullptr;
		}
		Modify();
		ErrorMsg.Reset();

        for (int32 PinIndex = Pins.Num() - 1; PinIndex >= 0; --PinIndex)
        {
            FName N = Pins[PinIndex]->PinName;

			if (N != UEdGraphSchema_K2::PN_Self &&	  //
				N != WorldContextPinName &&			  //
				N != OutPutObjectPinName &&			  //
				N != ClassPinName &&				  //
				N != UEdGraphSchema_K2::PN_Execute && //
				N != UEdGraphSchema_K2::PN_Then &&	  //
				N != NodeGuidStrName)				  //
			{
				Pins[PinIndex]->MarkAsGarbage();
				Pins.RemoveAt(PinIndex);
			}
		}

        AllocateDefaultPins();
    }
    Super::PinDefaultValueChanged(Pin);
}


//TMap<FName, UBtf_ExtendConstructObject_K2Node::FNodeHelperPinRedirectMapInfo> UBtf_ExtendConstructObject_K2Node::AsyncTaskPinRedirectMap;
//bool UBtf_ExtendConstructObject_K2Node::bAsyncTaskPinRedirectMapInitialized = false;
UK2Node::ERedirectType UBtf_ExtendConstructObject_K2Node::DoPinsMatchForReconstruction(
    const UEdGraphPin* NewPin,
    const int32 NewPinIndex,
    const UEdGraphPin* OldPin,
    const int32 OldPinIndex) const
{
    //todo DoPinsMatchForReconstruction?
    /*
    UE_LOG(LogTemp, Warning, TEXT("NewPin->PinName %s,  OldPin->PinName %s"), *NewPin->PinName.ToString(), *OldPin->PinName.ToString());

    if (NewPin->PinName != OldPin->PinName)
    {

    }

    if (OldPin->PinName == NAME_None)
    {
    }
    */
    /*
    if (GConfig && ProxyClass)
    {
        if (!bAsyncTaskPinRedirectMapInitialized)
        {
            bAsyncTaskPinRedirectMapInitialized = true;
            FConfigSection* PackageRedirects = GConfig->GetSectionPrivate(TEXT("/Script/Engine.Engine"), false, true, GEngineIni);
            for (FConfigSection::TIterator It(*PackageRedirects); It; ++It)
            {
                if (It.Key() == TEXT("K2AsyncTaskPinRedirects"))
                {
                    FString ProxyClassString;
                    FString OldPinString;
                    FString NewPinString;

                    FParse::Value(*It.Value().GetValue(), TEXT("ProxyClassName="), ProxyClassString);
                    FParse::Value(*It.Value().GetValue(), TEXT("OldPinName="), OldPinString);
                    FParse::Value(*It.Value().GetValue(), TEXT("NewPinName="), NewPinString);

                    UClass* RedirectProxyClass = FindObject<UClass>(ANY_PACKAGE, *ProxyClassString);
                    if (RedirectProxyClass)
                    {
                        FNodeHelperPinRedirectMapInfo& PinRedirectInfo = AsyncTaskPinRedirectMap.FindOrAdd(*OldPinString);
                        TArray<UClass*>& ProxyClassArray = PinRedirectInfo.OldPinToProxyClassMap.FindOrAdd(*NewPinString);
                        ProxyClassArray.AddUnique(RedirectProxyClass);
                    }
                }
            }
        }

        if (FNodeHelperPinRedirectMapInfo* PinRedirectInfo = AsyncTaskPinRedirectMap.Find(OldPin->PinName))
        {
            if (TArray<UClass*>* ProxyClassArray = PinRedirectInfo->OldPinToProxyClassMap.Find(NewPin->PinName))
            {
                for (UClass* RedirectedProxyClass : *ProxyClassArray)
                {
                    if (ProxyClass->IsChildOf(RedirectedProxyClass))
                    {
                        return UK2Node::ERedirectType_Name;
                    }
                }
            }
        }
    }
    */

    return Super::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
}


void UBtf_ExtendConstructObject_K2Node::AllocateDefaultPins()
{
	if (const auto OldPin = FindPin(UEdGraphSchema_K2::PN_Execute))
	{
		const int32 Idx = Pins.IndexOfByKey(OldPin);
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
		const int32 Idx = Pins.IndexOfByKey(OldPin);
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

    if (ProxyClass)
    {
        const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
        if (const UFunction* Function = GetFactoryFunction())
        {
            for (TFieldIterator<FProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
            {
                const FProperty* Param = *PropIt;

                const bool bIsFunctionOutput =						  //
                    Param->HasAnyPropertyFlags(CPF_OutParm) &&		  //
                    !Param->HasAnyPropertyFlags(CPF_ReferenceParm) && //
                    !Param->HasAnyPropertyFlags(CPF_ReturnParm);

                if (bIsFunctionOutput)
                {
                    UEdGraphPin* Pin = CreatePin(EGPD_Output, NAME_None, Param->GetFName());
                    K2Schema->ConvertPropertyToPinType(Param, /*out*/ Pin->PinType);
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

	for(auto& CurrentExtension : GetBlueprint()->GetExtensions())
	{
		if(CurrentExtension.IsA(UBtf_TaskForge::StaticClass()))
		{
			if(CurrentExtension.GetName().Contains(NodeGuid.ToString()))
			{
				TaskInstance = Cast<UBtf_TaskForge>(CurrentExtension);
				TaskInstance->OnPostPropertyChanged.BindLambda([this](FPropertyChangedEvent PropertyChangedEvent)
				{
					ReconstructNode();
				});
			}
		}
	}

	if(TaskInstance)
	{
		FindPin(NodeGuidStrName)->DefaultValue = ProxyClass->GetName() + NodeGuid.ToString();
		FindPin(NodeGuidStrName)->DefaultTextValue = FText::FromString(ProxyClass->GetName() + NodeGuid.ToString());
	}

    // Refresh the UI for the graph so the pin changes show up
    UEdGraph* Graph = GetGraph();
    Graph->NotifyGraphChanged();

    // Mark dirty
    FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}


void UBtf_ExtendConstructObject_K2Node::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    RestoreSplitPins(OldPins);

    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

    if (ProxyClass)
    {
        const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
        if (const UFunction* Function = GetFactoryFunction())
        {
            for (TFieldIterator<FProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
            {
                const FProperty* Param = *PropIt;
                const bool bIsFunctionOutput =						  //
                    Param->HasAnyPropertyFlags(CPF_OutParm) &&		  //
                    !Param->HasAnyPropertyFlags(CPF_ReferenceParm) && //
                    !Param->HasAnyPropertyFlags(CPF_ReturnParm);
                if (bIsFunctionOutput)
                {
                    UEdGraphPin* Pin = CreatePin(EGPD_Output, NAME_None, Param->GetFName());
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

	// Super::AllocateDefaultPins();
	GetGraph()->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}


UEdGraphPin* UBtf_ExtendConstructObject_K2Node::FindParamPin(const FString& ContextName, FName NativePinName, EEdGraphPinDirection Direction) const
{
    const FName ParamName = FName(*FString::Printf(TEXT("%s_%s"), *ContextName, *NativePinName.ToString()));
    return FindPin(ParamName, Direction);
}

UFunction* UBtf_ExtendConstructObject_K2Node::GetFactoryFunction() const
{
    if (ProxyFactoryClass == nullptr)
    {
        UE_LOG(LogBlueprint, Error, TEXT("ProxyFactoryClass null in %s. Was a class deleted or saved on a non promoted build?"), *GetFullName());
        return nullptr;
    }

    FMemberReference FunctionRef;
    FunctionRef.SetExternalMember(ProxyFactoryFunctionName, ProxyFactoryClass);

    UFunction* FactoryFunction = FunctionRef.ResolveMember<UFunction>(GetBlueprint());

    if (FactoryFunction == nullptr)
    {
        FactoryFunction = ProxyFactoryClass->FindFunctionByName(ProxyFactoryFunctionName);
        UE_CLOG(
            FactoryFunction == nullptr,
            LogBlueprint,
            Error,
            TEXT("FactoryFunction %s null in %s. Was a class deleted or saved on a non promoted build?"),
            *ProxyFactoryFunctionName.ToString(),
            *GetFullName());
    }

    return FactoryFunction;
}


void UBtf_ExtendConstructObject_K2Node::RefreshNames(TArray<FBtf_NameSelect>& NameArray, const bool bRemoveNone) const
{
    if (bRemoveNone)
    {
        NameArray.RemoveAll([](const FBtf_NameSelect& A) { return A.Name == NAME_None; });
    }

    TArray<FBtf_NameSelect> CopyNameArray = NameArray;

    CopyNameArray.Sort([](const FBtf_NameSelect& A, const FBtf_NameSelect& B) { return GetTypeHash(A.Name) < GetTypeHash(B.Name); });

	auto UniquePred = [&Arr = NameArray](const FBtf_NameSelect& A, const FBtf_NameSelect& B)
	{
		if (A.Name != NAME_None && A.Name == B.Name)
		{
			const int32 ID = Arr.FindLast(B.Name);
			Arr.RemoveAt(ID, 1, EAllowShrinking::No);
			return true;
		}
		return false;
	};

    Algo::Unique(CopyNameArray, UniquePred);
}

void UBtf_ExtendConstructObject_K2Node::ResetPinByNames(TSet<FName>& NameArray)
{
    for (const FName OldPinReference : NameArray)
    {
        UEdGraphPin* OldPin = FindPin(OldPinReference);
        if (OldPin)
        {
            Pins.Remove(OldPin);
            OldPin->MarkAsGarbage();
        }
    }
    NameArray.Reset();
}

void UBtf_ExtendConstructObject_K2Node::ResetPinByNames(TArray<FBtf_NameSelect>& NameArray)
{
    for (const FName OldPinReference : NameArray)
    {
        UEdGraphPin* OldPin = FindPin(OldPinReference);
        if (OldPin)
        {
            Pins.Remove(OldPin);
            OldPin->MarkAsGarbage();
        }
    }
    NameArray.Reset();
}

void UBtf_ExtendConstructObject_K2Node::AddAutoCallFunction(const FName PinName)
{
    const int32 OldNum = AutoCallFunction.Num();
    AutoCallFunction.AddUnique(PinName);
    if (OldNum - AutoCallFunction.Num() != 0)
    {
        if (!ExecFunction.Remove(PinName))
        {
            if (UEdGraphPin* Pin = FindPin(PinName))
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
    const int32 OldNum = ExecFunction.Num();
    ExecFunction.AddUnique(PinName);
    if (OldNum - ExecFunction.Num() != 0)
    {
        if (!AutoCallFunction.Remove(PinName))
        {
            if (UEdGraphPin* Pin = FindPin(PinName))
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
    const int32 OldNum = OutDelegate.Num();
    OutDelegate.AddUnique(PinName);
    if (OldNum - OutDelegate.Num() != 0)
    {
        if (InDelegate.Remove(PinName))
        {
            UEdGraphPin* Pin = FindPinChecked(PinName);
            RemovePin(Pin);
            Pin->MarkAsGarbage();
        }
        ReconstructNode();
    }
}

void UBtf_ExtendConstructObject_K2Node::AddInputDelegate(const FName PinName)
{
    const int32 OldNum = InDelegate.Num();
    InDelegate.AddUnique(PinName);
    if (OldNum - InDelegate.Num() != 0)
    {
        if (OutDelegate.Remove(PinName))
        {
            UEdGraphPin* Pin = FindPinChecked(PinName);
            RemovePin(Pin);
            Pin->MarkAsGarbage();
        }
        ReconstructNode();
    }
}

void UBtf_ExtendConstructObject_K2Node::AddSpawnParam(const FName PinName)
{
    const int32 OldNum = SpawnParam.Num();
    SpawnParam.AddUnique(PinName);
    if (OldNum - SpawnParam.Num() != 0)
    {
        ReconstructNode();
    }
}

void UBtf_ExtendConstructObject_K2Node::RemoveNodePin(const FName PinName)
{
    const UEdGraphPin* Pin = FindPin(PinName);
    check(Pin);
    if (!ExecFunction.Remove(PinName))
    {
        if (!AutoCallFunction.Remove(PinName))
        {
            if (!OutDelegate.Remove(PinName))
            {
                if (!InDelegate.Remove(PinName))
                {
                    SpawnParam.Remove(PinName);
                }
            }
        }
    }

    Modify();
    FString NameStr = PinName.ToString();
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

    //RemovePin(Pin);
    //Pin->MarkPendingKill();
    ReconstructNode();
}


void UBtf_ExtendConstructObject_K2Node::GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const
{
    TMap<FString, FStringFormatArg> Args;

    if (ProxyClass)
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

    const FString FullPinName = FString::Format(TEXT("{ProxyClass}{ProxyClassSeparator}{ProxyFactoryFunction}{ProxyFactoryFunctionSeparator}{PinName}"), Args);
    RedirectPinNames.Add(FullPinName);
}


void UBtf_ExtendConstructObject_K2Node::GenerateFactoryFunctionPins(UClass* InClass)
{
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
    if (const UFunction* Function = GetFactoryFunction())
    {
        //const FString& IgnorePropertyListStr = Function->GetMetaData(FName(TEXT("HideSpawnParms")));
        //TArray<FString> IgnorePropertyList;
        //if (!IgnorePropertyListStr.IsEmpty())
        //{
        //	IgnorePropertyListStr.ParseIntoArray(IgnorePropertyList, TEXT(","), true);
        //}

        bool bAllPinsGood = true;
//++CK
        //TSet<FName> PinsToHide;
        TSet<FName> PinsToHide = Get_PinsHiddenByDefault();
//--CK

        FBlueprintEditorUtils::GetHiddenPinsForFunction(GetGraph(), Function, PinsToHide);
        for (TFieldIterator<FProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
        {
            FProperty* const Property = *PropIt;
            const bool bIsFunctionInput = !Property->HasAnyPropertyFlags(CPF_OutParm) || Property->HasAnyPropertyFlags(CPF_ReferenceParm);
            if (!bIsFunctionInput)
            {
                continue; // skip function output, it's internal node data
            }

            //WorldContextPin as self

            FCreatePinParams PinParams;
            PinParams.bIsReference = Property->HasAnyPropertyFlags(CPF_ReferenceParm) && bIsFunctionInput;

            UEdGraphPin* Pin = nullptr;
            const FName N = Property->GetFName();
            if (N == WorldContextPinName || N == UEdGraphSchema_K2::PSC_Self)
            {
                if (UEdGraphPin* OldPin = FindPin(WorldContextPinName))
                {
                    Pin = OldPin;
                }
                if (UEdGraphPin* OldPin = FindPin(UEdGraphSchema_K2::PSC_Self))
                {
                    Pin = OldPin;
                }
                if (!bOwnerContextPin)
                {
                    if (const FObjectProperty* Prop = CastFieldChecked<FObjectProperty>(Property))
                    {
                        const auto BPClass = GetBlueprintClassFromNode();
                        bSelfContext = BPClass->IsChildOf(Prop->PropertyClass);
                        if (bSelfContext)
                        {
                            Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UEdGraphSchema_K2::PSC_Self, UEdGraphSchema_K2::PN_Self);
                            Pin->PinFriendlyName = FText::FromName(WorldContextPinName);
                            if (!bOwnerContextPin && Pin->LinkedTo.Num() == 0)
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
                        bOwnerContextPin = true;
                    }
                }
            }
            else if (N == ClassPinName)
            {
                if (UEdGraphPin* OldPin = GetClassPin())
                {
                    Pin = OldPin;
                    Pin->bHidden = false;
                }
            }

			if (Pin == nullptr)
			{
				Pin = CreatePin(EGPD_Input, NAME_None, Property->GetFName(), PinParams);
			}
			else
			{
				const int32 Idx = Pins.IndexOfByKey(Pin);
				Pins.RemoveAt(Idx, 1, EAllowShrinking::No);
				Pins.Add(Pin);
			}

            check(Pin);
            const bool bPinGood = (Pin && K2Schema->ConvertPropertyToPinType(Property, Pin->PinType));

            if (bPinGood)
            {
                //Flag pin as read only for const reference property
                Pin->bDefaultValueIsIgnored = Property->HasAllPropertyFlags(CPF_ConstParm | CPF_ReferenceParm) &&
                    (!Function->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm) || Pin->PinType.IsContainer());

                const bool bAdvancedPin = Property->HasAllPropertyFlags(CPF_AdvancedDisplay);
                Pin->bAdvancedView = bAdvancedPin;
                if (bAdvancedPin && (ENodeAdvancedPins::NoPins == AdvancedPinDisplay))
                {
                    AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
                }

                FString ParamValue;
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

            bAllPinsGood = bAllPinsGood && bPinGood;
        }

        if (bAllPinsGood)
        {
            UEdGraphPin* ClassPin = FindPinChecked(ClassPinName);
            ClassPin->DefaultObject = InClass;
            ClassPin->DefaultValue.Empty();
            check(GetWorldContextPin());
        }
    }

	if (UEdGraphPin* OldPin = FindPin(OutPutObjectPinName))
	{
		const int32 Idx = Pins.IndexOfByKey(OldPin);
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
//++CK
    _PinMetadataMap.Reset();
//--CK

    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
    UObject* const ClassDefaultObject = InClass->GetDefaultObject(true);

    for (const FName ParamName : SpawnParam)
    {
        if (ParamName != NAME_None)
        {
            if (const FProperty* Property = InClass->FindPropertyByName(ParamName))
            {
                UEdGraphPin* Pin = CreatePin(EGPD_Input, NAME_None, ParamName);
                check(Pin);
                K2Schema->ConvertPropertyToPinType(Property, Pin->PinType);

				if (ClassDefaultObject && K2Schema->PinDefaultValueIsEditable(*Pin))
				{
					FString DefaultValueAsString;
					const bool bDefaultValueSet =
						FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(ClassDefaultObject), DefaultValueAsString, this);
					check(bDefaultValueSet);
					K2Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValueAsString);
				}
				K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);

//++CK
                if (const auto* MetaDataMap = Property->GetMetaDataMap())
                {
                    for (const auto& MetaDataKvp : *MetaDataMap)
                    {
                        _PinMetadataMap.FindOrAdd(Pin->PinName).Add(MetaDataKvp.Key, MetaDataKvp.Value);
                    }
                }
//--CK
            }
        }
    }
}


void UBtf_ExtendConstructObject_K2Node::GenerateAutoCallFunctionPins(UClass* InClass)
{
    for (const FName FnName : AutoCallFunction)
    {
        if (FnName != NAME_None)
        {
            CreatePinsForClassFunction(InClass, FnName, true);
            if (UEdGraphPin* ExecPin = FindPin(FnName, EGPD_Input))
            {
                // ++CK Hide all Autocall function input pins by default
                ExecPin->bHidden = true;
                // --CK
                ExecPin->bNotConnectable = true;
                ExecPin->bDefaultValueIsReadOnly = true;
                ExecPin->bDefaultValueIsIgnored = true;
                FEdGraphPinType& PinType = ExecPin->PinType;
                PinType.PinCategory = UEdGraphSchema_K2::PN_EntryPoint; //NAME_None;
            }
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GenerateExecFunctionPins(UClass* InClass)
{
    for (const FName FnName : ExecFunction)
    {
        if (FnName != NAME_None)
        {
            CreatePinsForClassFunction(InClass, FnName, false);
        }
    }
}

void UBtf_ExtendConstructObject_K2Node::GenerateInputDelegatePins(UClass* InClass)
{
    //const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
    for (FName DelegateName : InDelegate)
    {
        if (DelegateName != NAME_None)
        {
            if (const FProperty* Property = InClass->FindPropertyByName(DelegateName))
            {
                const FMulticastDelegateProperty* DelegateProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);

                FString DelegateNameStr = DelegateName.ToString();

                FCreatePinParams PinParams;
                PinParams.bIsReference = true;
                PinParams.bIsConst = true;

                if (UEdGraphPin* DelegatePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Delegate, DelegateName, PinParams))
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
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
    for (FName DelegateName : OutDelegate)
    {
        if (DelegateName != NAME_None)
        {
            if (const FProperty* Property = InClass->FindPropertyByName(DelegateName))
            {
                const FMulticastDelegateProperty* DelegateProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);
                FString DelegateNameStr = DelegateName.ToString();

                UEdGraphPin* ExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, DelegateName);
                ExecPin->PinToolTip = DelegateProperty->GetToolTipText().ToString();
                ExecPin->PinFriendlyName = FText::FromString(DelegateNameStr);

                if (const UFunction* DelegateSignatureFunction = DelegateProperty->SignatureFunction)
                {
                    for (TFieldIterator<FProperty> PropIt(DelegateSignatureFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
                    {
                        const FProperty* Param = *PropIt;
                        const bool bIsFunctionInput = !Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm);
                        if (bIsFunctionInput)
                        {
                            const FName ParamFullName = FName(*FString::Printf(TEXT("%s_%s"), *DelegateNameStr, *Param->GetFName().ToString()));
                            UEdGraphPin* Pin = CreatePin(EGPD_Output, NAME_None, ParamFullName);
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

void UBtf_ExtendConstructObject_K2Node::CreatePinsForClass(UClass* InClass, const bool bFullRefresh)
{
    if (bFullRefresh || InClass == nullptr)
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

        TSet<FName> ContextPin;
        ContextPin.Append({WorldContextPinName, UEdGraphSchema_K2::PSC_Self});
        ResetPinByNames(ContextPin);
    }

    GenerateFactoryFunctionPins(InClass);

    if (InClass == nullptr)
    {
        return;
    }

    const UObject* const ClassDefaultObject = InClass->GetDefaultObject(true);
    check(ClassDefaultObject);
    CollectSpawnParam(InClass, bFullRefresh);

    check(ClassDefaultObject);
    GenerateSpawnParamPins(InClass);
    GenerateAutoCallFunctionPins(InClass);
    GenerateExecFunctionPins(InClass);
    GenerateInputDelegatePins(InClass);
    GenerateOutputDelegatePins(InClass);

    {
        const auto Pred = [&](const FBtf_NameSelect& A) { return A.Name != NAME_None && FindPin(A.Name) == nullptr; };
        const auto ValidPropertyPred = [InCl = InClass](const FName& Name)
        {
            if (Name != NAME_None)
            {
                const FProperty* Property = InCl->FindPropertyByName(Name);
                if (Property == nullptr)
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
                const UFunction* Property = InCl->FindFunctionByName(Name);
                if (Property == nullptr)
                {
                    return true;
                }
            }
            return false;
        };

        if (AllParam.Num())
        {
            TArray<FName> Arr = AllParam.Array();
            const int32 Count = Arr.RemoveAll(ValidPropertyPred);
            if (Count)
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
            TArray<FName> Arr = AllDelegates.Array();
            const int32 Count = Arr.RemoveAll(ValidPropertyPred);
            if (Count)
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
                TArray<FName> Arr = AllFunctions.Array();
                const int32 Count = Arr.RemoveAll(ValidFunctionPred);
                if (Count)
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
                    if (const UFunction* Property = InCl->FindFunctionByName(Name))
                    {
                        return Property->HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Const);
                    }
                }
                return false;
            };

            {
                TArray<FName> Arr = AllFunctionsExec.Array();
                const int32 Count = Arr.RemoveAll(PureFunctionPred);
                if (Count)
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

void UBtf_ExtendConstructObject_K2Node::CreatePinsForClassFunction(UClass* InClass, const FName FnName, const bool bRetValPins)
{
    if (const UFunction* LocFunction = InClass->FindFunctionByName(FnName))
    {
        const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

        UEdGraphPin* ExecPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, LocFunction->GetFName());
        ExecPin->PinToolTip = LocFunction->GetToolTipText().ToString();

        {
            FString FunctionNameStr =										 //
                LocFunction->HasMetaData(FBlueprintMetadata::MD_DisplayName) //
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

        bool bAllPinsGood = true;
        for (TFieldIterator<FProperty> PropIt(LocFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
        {
            FProperty* Param = *PropIt;
            const bool bIsFunctionInput =
                !Param->HasAnyPropertyFlags(CPF_ReturnParm) && (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm));
            const EEdGraphPinDirection Direction = bIsFunctionInput ? EGPD_Input : EGPD_Output;

            if (Direction == EGPD_Output && !bRetValPins) continue;

            UEdGraphNode::FCreatePinParams PinParams;

            PinParams.bIsReference = Param->HasAnyPropertyFlags(CPF_ReferenceParm) && bIsFunctionInput;

            const FName ParamFullName = FName(*FString::Printf(TEXT("%s_%s"), *FnName.ToString(), *Param->GetFName().ToString()));
            UEdGraphPin* Pin = CreatePin(Direction, NAME_None, ParamFullName, PinParams);
            const bool bPinGood = (Pin && K2Schema->ConvertPropertyToPinType(Param, Pin->PinType));

            if (bPinGood)
            {
                Pin->bDefaultValueIsIgnored =											 //
                    Param->HasAllPropertyFlags(/*CPF_ConstParm |*/ CPF_ReferenceParm) && //
                    (!LocFunction->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm) || Pin->PinType.IsContainer());

                FString ParamValue;
                if (K2Schema->FindFunctionParameterDefaultValue(LocFunction, Param, ParamValue))
                {
                    K2Schema->SetPinAutogeneratedDefaultValue(Pin, ParamValue);
                }
                else
                {
                    K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
                }

                const FString& PinDisplayName = Param->HasMetaData(FBlueprintMetadata::MD_DisplayName) //
                    ? Param->GetMetaData(FBlueprintMetadata::MD_DisplayName)						   //
                    : Param->GetFName().ToString();

                if (PinDisplayName == FString(TEXT("ReturnValue")))
                {
                    const FString ReturnPropertyName = FString::Printf(TEXT("%s\n%s"), *FnName.ToString(), *PinDisplayName);
                    Pin->PinFriendlyName = FText::FromString(ReturnPropertyName);
                }
                else if (!PinDisplayName.IsEmpty())
                {
                    Pin->PinFriendlyName = FText::FromString(PinDisplayName);
                }
                else if (LocFunction->GetReturnProperty() == Param && LocFunction->HasMetaData(FBlueprintMetadata::MD_ReturnDisplayName))
                {
                    const FString ReturnPropertyName = FString::Printf(
                        TEXT("%s_%s"),
                        *FnName.ToString(),
                        *(LocFunction->GetMetaDataText(FBlueprintMetadata::MD_ReturnDisplayName).ToString()));
                    Pin->PinFriendlyName = FText::FromString(ReturnPropertyName);
                }
                Pin->PinToolTip = ParamFullName.ToString();
            }

            bAllPinsGood = bAllPinsGood && bPinGood;
        }
        check(bAllPinsGood);
    }
}


void UBtf_ExtendConstructObject_K2Node::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);
    if (ProxyClass == nullptr)
    {
        CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: ProxyClass nullptr error. @@"), this);
        return;
    }

    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
    check(SourceGraph && Schema);
    bool bIsErrorFree = true;

    // ------------------------------------------------------------------------------------------
    //  Create a call to factory the proxy object error
    // ------------------------------------------------------------------------------------------

    UK2Node_CallFunction* const CreateProxyObject_Node = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    CreateProxyObject_Node->FunctionReference.SetExternalMember(ProxyFactoryFunctionName, ProxyFactoryClass);
    CreateProxyObject_Node->AllocateDefaultPins();
    if (CreateProxyObject_Node->GetTargetFunction() == nullptr)
    {
        const FText ClassName = ProxyFactoryClass ? FText::FromString(ProxyFactoryClass->GetName()) : LOCTEXT("MissingClassString", "Unknown Class");
        const FString FormattedMessage = //
            FText::Format(
                LOCTEXT("AsyncTaskErrorFmt", "BaseAsyncTask: Missing function {0} from class {1} for async task @@"),
                FText::FromString(ProxyFactoryFunctionName.GetPlainNameString()),
                ClassName)
                .ToString();

        CompilerContext.MessageLog.Error(*FormattedMessage, this);
        return;
    }

    bIsErrorFree &= CompilerContext
                        .MovePinLinksToIntermediate( //
                            *FindPinChecked(UEdGraphSchema_K2::PN_Execute),
                            *CreateProxyObject_Node->FindPinChecked(UEdGraphSchema_K2::PN_Execute))
                        .CanSafeConnect();
    if (bSelfContext)
    {
        UK2Node_Self* const Self_Node = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(this, SourceGraph);
        Self_Node->AllocateDefaultPins();
        UEdGraphPin* SelfPin = Self_Node->FindPinChecked(UEdGraphSchema_K2::PN_Self);
        bIsErrorFree &= Schema->TryCreateConnection(SelfPin, CreateProxyObject_Node->FindPinChecked(WorldContextPinName));
        if (!bIsErrorFree)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Pin Self Context Error. @@"), this);
        }
    }

    for (UEdGraphPin* CurrentPin : Pins)
    {
        if (CurrentPin->PinName != UEdGraphSchema_K2::PSC_Self && FNodeHelper::ValidDataPin(CurrentPin, EGPD_Input))
        {
            if (UEdGraphPin* DestPin = CreateProxyObject_Node->FindPin(CurrentPin->PinName))
            {
                bIsErrorFree &= CompilerContext.CopyPinLinksToIntermediate(*CurrentPin, *DestPin).CanSafeConnect();
                if (!bIsErrorFree)
                {
                    CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Create a call to factory the proxy object error. @@"), this);
                }
            }
        }
    }

	CreateProxyObject_Node->FindPin(NodeGuidStrName)->DefaultValue = NodeGuid.ToString();

    // Expose Async Task Proxy object
    UEdGraphPin* const PinProxyObject = CreateProxyObject_Node->GetReturnValuePin();
    UEdGraphPin* Cast_Output = PinProxyObject;
    if (PinProxyObject->PinType.PinSubCategoryObject.Get() != ProxyClass)
    {
        //Create Cast Node
        UK2Node_DynamicCast* CastNode = CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, SourceGraph);
        CastNode->SetPurity(true);
        CastNode->TargetType = ProxyClass;
        CastNode->AllocateDefaultPins();
        bIsErrorFree &= Schema->TryCreateConnection(PinProxyObject, CastNode->GetCastSourcePin());
        Cast_Output = CastNode->GetCastResultPin();
        check(Cast_Output);
    }
    UEdGraphPin* const OutputObjectPin = FindPinChecked(OutPutObjectPinName, EGPD_Output);
    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*OutputObjectPin, *Cast_Output).CanSafeConnect();
    if (!bIsErrorFree)
    {
        CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: OutObjectPin Error. @@"), this);
    }

    // ------------------------------------------------------------------------------------------
    // Gather output parameters and pair them with local variables
    // ------------------------------------------------------------------------------------------
    TArray<FNodeHelper::FOutputPinAndLocalVariable> VariableOutputs;
    for (UEdGraphPin* CurrentPin : Pins)
    {
        if (OutputObjectPin != CurrentPin && FNodeHelper::ValidDataPin(CurrentPin, EGPD_Output))
        {
            const FEdGraphPinType& PinType = CurrentPin->PinType;

            UK2Node_TemporaryVariable* TempVarOutput = //
                CompilerContext.SpawnInternalVariable(
                    this,
                    PinType.PinCategory,
                    PinType.PinSubCategory,
                    PinType.PinSubCategoryObject.Get(),
                    PinType.ContainerType,
                    PinType.PinValueType);

            bIsErrorFree &= TempVarOutput->GetVariablePin() != nullptr;
            bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *TempVarOutput->GetVariablePin()).CanSafeConnect();
            VariableOutputs.Add(FNodeHelper::FOutputPinAndLocalVariable(CurrentPin, TempVarOutput));
        }
    }

    const FName IsValidFuncName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid);

    UEdGraphPin* OriginalThenPin = FindPinChecked(UEdGraphSchema_K2::PN_Then);
    UEdGraphPin* LastThenPin = CreateProxyObject_Node->FindPinChecked(UEdGraphSchema_K2::PN_Then);
    UK2Node_IfThenElse* ValidateProxyNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
    ValidateProxyNode->AllocateDefaultPins();
    {
        UK2Node_CallFunction* IsValidFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

        IsValidFuncNode->FunctionReference.SetExternalMember(IsValidFuncName, UKismetSystemLibrary::StaticClass());
        IsValidFuncNode->AllocateDefaultPins();
        UEdGraphPin* IsValidInputPin = IsValidFuncNode->FindPinChecked(TEXT("Object"));

        bIsErrorFree &= Schema->TryCreateConnection(PinProxyObject, IsValidInputPin);
        bIsErrorFree &= Schema->TryCreateConnection(IsValidFuncNode->GetReturnValuePin(), ValidateProxyNode->GetConditionPin());
        bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, ValidateProxyNode->GetExecPin());
        bIsErrorFree &= CompilerContext.CopyPinLinksToIntermediate(*OriginalThenPin, *ValidateProxyNode->GetElsePin()).CanSafeConnect();

        LastThenPin = ValidateProxyNode->GetThenPin();
    }

    // ------------------------------------------------------------------------------------------
    //	Create a InDelegate Assign
    // ------------------------------------------------------------------------------------------
    for (FName DelegateName : InDelegate)
    {
        if (DelegateName == NAME_None) continue;

        if (FProperty* Property = ProxyClass->FindPropertyByName(DelegateName))
        {
            FMulticastDelegateProperty* CurrentProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);
            if (CurrentProperty && CurrentProperty->GetFName() == DelegateName)
            {
                UEdGraphPin* InDelegatePin = FindPinChecked(DelegateName, EGPD_Input);
                if (InDelegatePin->LinkedTo.Num() != 0)
                {
                    UK2Node_AssignDelegate* AssignDelegate = CompilerContext.SpawnIntermediateNode<UK2Node_AssignDelegate>(this, SourceGraph);
                    AssignDelegate->SetFromProperty(CurrentProperty, false, CurrentProperty->GetOwnerClass());
                    AssignDelegate->AllocateDefaultPins();

                    bIsErrorFree &= Schema->TryCreateConnection(AssignDelegate->FindPinChecked(UEdGraphSchema_K2::PN_Self), Cast_Output);
                    bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, AssignDelegate->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
                    UEdGraphPin* DelegatePin = AssignDelegate->GetDelegatePin();

                    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*InDelegatePin, *DelegatePin).CanSafeConnect();
                    if (!bIsErrorFree)
                    {
                        CompilerContext.MessageLog.Error(*LOCTEXT("Invalid_DelegateProperties", "Error @@").ToString(), this);
                    }
                    LastThenPin = AssignDelegate->FindPinChecked(UEdGraphSchema_K2::PN_Then);
                }
            }
        }
        else
        {
            bIsErrorFree = false;
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
            continue;
        }
    }


    // ------------------------------------------------------------------------------------------
    //	Create a OutDelegate
    // ------------------------------------------------------------------------------------------
    for (FName DelegateName : OutDelegate)
    {
        if (DelegateName == NAME_None) continue;

        if (FProperty* Property = ProxyClass->FindPropertyByName(DelegateName))
        {
            FMulticastDelegateProperty* CurrentProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);
            if (CurrentProperty && CurrentProperty->GetFName() == DelegateName)
            {
                UEdGraphPin* DelegateProperty = FindPin(CurrentProperty->GetFName());
                if (DelegateProperty)
                {
                    bIsErrorFree &= FNodeHelper::HandleDelegateImplementation(
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
                    bIsErrorFree = false;
                }
                if (!bIsErrorFree)
                {
                    CompilerContext.MessageLog.Error(*LOCTEXT("Invalid_DelegateProperties", "@@").ToString(), this);
                }
            }
        }
        else
        {
            bIsErrorFree = false;
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
            continue;
        }
    }

    if (!bIsErrorFree)
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

    bIsErrorFree &= ConnectSpawnProperties(ProxyClass, Schema, CompilerContext, SourceGraph, LastThenPin, PinProxyObject);
    if (!bIsErrorFree)
    {
        CompilerContext.MessageLog.Error(TEXT("ConnectSpawnProperties error. @@"), this);
    }

	// --------------------------------------------------------------------------------------
	// Create the custom pins
	// --------------------------------------------------------------------------------------
	// CustomPins = GetInstanceOrDefaultObject()->GetCustomOutputPins();
	if(!CustomPins.IsEmpty())
	{
	    //Find the delegate property
		FProperty* DelegateProperty = ProxyClass->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UBtf_TaskForge, OnCustomPinTriggered));
	    FMulticastDelegateProperty* MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(DelegateProperty);

	    if (!MulticastDelegateProperty)
	    {
	        CompilerContext.MessageLog.Error(*FString::Printf(TEXT("Failed to find delegate property for %s."), *GetNodeTitle(ENodeTitleType::FullTitle).ToString()));
	        BreakAllNodeLinks();
	        return;
	    }

	    //Define the output pins that correspond to each possible delegate value
		//Varian: I don't think this is needed?
	    TArray<FNodeHelper::FOutputPinAndLocalVariable> VariableOutputs2;
	    for (UEdGraphPin* Pin : Pins)
	    {
	        if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
	        {
	            FNodeHelper::FOutputPinAndLocalVariable OutputMapping;
	            OutputMapping.OutputPin = Pin;
	            OutputMapping.TempVar = nullptr; // No local variable needed
	            VariableOutputs2.Add(OutputMapping);
	        }
	    }

		/**The regular HandleDelegateImplementation is more strict and I didn't want to modify it.
		 * So we use our CustomPins implementation instead. */
	    const bool bSuccess = FNodeHelper::HandleCustomPinsImplementation(
	        MulticastDelegateProperty,
	        VariableOutputs2,
	        Cast_Output,
	        LastThenPin,
	        this,
	        SourceGraph,
	        CustomPins,
	        CompilerContext
	    );

	    if (!bSuccess)
	    {
	        CompilerContext.MessageLog.Error(*FString::Printf(TEXT("Failed to handle delegate for %s."), *GetNodeTitle(ENodeTitleType::FullTitle).ToString()));
	    }
	}


    // --------------------------------------------------------------------------------------
    // Create a call to activate the proxy object
    // --------------------------------------------------------------------------------------
    for (FName FunctionName : AutoCallFunction)
    {
        if (FunctionName == NAME_None) continue;
        {
            const UFunction* FindFunc = ProxyClass->FindFunctionByName(FunctionName);
            if (FindFunc == nullptr)
            {
                bIsErrorFree = false;
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
                continue;
            }
        }

        UK2Node_CallFunction* const CallFunction_Node = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
        CallFunction_Node->FunctionReference.SetExternalMember(FunctionName, ProxyClass);
        CallFunction_Node->AllocateDefaultPins();

        /*
        if (bIsPureFunc)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need To Refresh Node. @@"), this);
            continue;
        }
        */

        UEdGraphPin* ActivateCallSelfPin = Schema->FindSelfPin(*CallFunction_Node, EGPD_Input);
        check(ActivateCallSelfPin);
        bIsErrorFree &= Schema->TryCreateConnection(Cast_Output, ActivateCallSelfPin);
        if (!bIsErrorFree)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Create a call to activate the proxy object error. @@"), this);
        }

        const bool bIsPureFunc = CallFunction_Node->IsNodePure();
        if (!bIsPureFunc)
        {
            UEdGraphPin* ActivateExecPin = CallFunction_Node->FindPinChecked(UEdGraphSchema_K2::PN_Execute);
            UEdGraphPin* ActivateThenPin = CallFunction_Node->FindPinChecked(UEdGraphSchema_K2::PN_Then);
            bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, ActivateExecPin);
            if (!bIsErrorFree)
            {
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Create a call to activate the proxy object error. @@"), this);
            }
            LastThenPin = ActivateThenPin;
        }

        UFunction* EventSignature = CallFunction_Node->GetTargetFunction();
        check(EventSignature);
        for (TFieldIterator<FProperty> PropIt(EventSignature); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
        {
            const FProperty* Param = *PropIt;

            const bool bIsFunctionInput =					   //
                !Param->HasAnyPropertyFlags(CPF_ReturnParm) && //
                (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm));

            const EEdGraphPinDirection Direction = bIsFunctionInput ? EGPD_Input : EGPD_Output;
            UEdGraphPin* InNodePin = FindParamPin(FunctionName.ToString(), Param->GetFName(), Direction);

            if (InNodePin)
            {
                UEdGraphPin* InFunctionPin = CallFunction_Node->FindPinChecked(Param->GetFName(), Direction);
                if (bIsFunctionInput)
                {
                    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*InNodePin, *InFunctionPin).CanSafeConnect();
                }
                else
                {
                    FNodeHelper::FOutputPinAndLocalVariable* OutputPair = VariableOutputs.FindByKey(InNodePin);
                    check(OutputPair);

                    UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
                    AssignNode->AllocateDefaultPins();
                    bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, AssignNode->GetExecPin());

                    bIsErrorFree &= Schema->TryCreateConnection(OutputPair->TempVar->GetVariablePin(), AssignNode->GetVariablePin());
                    AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());
                    bIsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), InFunctionPin);
                    AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

                    LastThenPin = AssignNode->GetThenPin();
                }
            }
            else
            {
                bIsErrorFree = false;
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
            }
        }

        if (!bIsErrorFree)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Create a call to activate the proxy object error. @@"), this);
        }
    }


    // --------------------------------------------------------------------------------------
    // Create a call to Expand Functions the proxy object
    // --------------------------------------------------------------------------------------
    for (FName FunctionName : ExecFunction)
    {
        if (FunctionName == NAME_None) continue;
        {
            const UFunction* FindFunc = ProxyClass->FindFunctionByName(FunctionName);
            if (FindFunc == nullptr)
            {
                bIsErrorFree = false;
                CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
                continue;
            }
        }

        UEdGraphPin* ExecPin = FindPinChecked(FunctionName, EGPD_Input);
        UK2Node_CallFunction* IsValidFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);


        UK2Node_IfThenElse* IfThenElse = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);

        IsValidFuncNode->FunctionReference.SetExternalMember(IsValidFuncName, UKismetSystemLibrary::StaticClass());
        IsValidFuncNode->AllocateDefaultPins();
        IfThenElse->AllocateDefaultPins();

        UEdGraphPin* IsValidInputPin = IsValidFuncNode->FindPinChecked(TEXT("Object"));
        bIsErrorFree &= Schema->TryCreateConnection(Cast_Output, IsValidInputPin);
        bIsErrorFree &= Schema->TryCreateConnection(IsValidFuncNode->GetReturnValuePin(), IfThenElse->GetConditionPin());

        bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*ExecPin, *IfThenElse->GetExecPin()).CanSafeConnect();

        UK2Node_CallFunction* const CallFunction_Node = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
        CallFunction_Node->FunctionReference.SetExternalMember(FunctionName, ProxyClass);
        CallFunction_Node->AllocateDefaultPins();

        const bool bIsPureFunc = CallFunction_Node->IsNodePure();
        if (bIsPureFunc)
        {
            CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need To Refresh Node. @@"), this);
            continue;
        }

        UEdGraphPin* CallSelfPin = Schema->FindSelfPin(*CallFunction_Node, EGPD_Input);
        check(CallSelfPin);

        bIsErrorFree &= Schema->TryCreateConnection(Cast_Output, CallSelfPin);

        UEdGraphPin* FunctionExecPin = CallFunction_Node->FindPinChecked(UEdGraphSchema_K2::PN_Execute);

        check(FunctionExecPin && ExecPin);

        bIsErrorFree &= Schema->TryCreateConnection(IfThenElse->GetThenPin(), FunctionExecPin);

        UFunction* EventSignature = CallFunction_Node->GetTargetFunction();
        for (TFieldIterator<FProperty> PropIt(EventSignature); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
        {
            const FProperty* Param = *PropIt;
            const bool bIsFunctionInput =					   //
                !Param->HasAnyPropertyFlags(CPF_ReturnParm) && //
                (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm));
            if (bIsFunctionInput)
            {
                UEdGraphPin* InNodePin = FindParamPin(FunctionName.ToString(), Param->GetFName(), EGPD_Input);
                if (InNodePin)
                {
                    UEdGraphPin* InFunctionPin = CallFunction_Node->FindPinChecked(Param->GetFName(), EGPD_Input);

                    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*InNodePin, *InFunctionPin).CanSafeConnect();
                }
                else
                {
                    bIsErrorFree = false;
                    CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Need Refresh Node. @@"), this);
                }
            }
        }
    }

    if (!bIsErrorFree)
    {
        CompilerContext.MessageLog.Error(TEXT("ExtendConstructObject: Create a call to Expand Functions the proxy object error. @@"), this);
    }


    // --------------------------------------------------------------------------------------
    // Move the connections from the original node then pin to the last internal then pin
    // --------------------------------------------------------------------------------------
    bIsErrorFree &= CompilerContext.CopyPinLinksToIntermediate(*OriginalThenPin, *LastThenPin).CanSafeConnect();
    if (!bIsErrorFree)
    {
        CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "ExtendConstructObject: Internal connection error. @@").ToString(), this);
    }

    /*
    if (const UEdGraphPin* OwnerPin = GetWorldContextPin()) //todo need early validation?
    {
        const auto BPClass = GetBlueprintClassFromNode();
        const bool bNeedContext = !CheckContextClass(BPClass);
        if (bNeedContext)
        {
            if (OwnerPin->LinkedTo.Num() == 0)
            {
                bIsErrorFree = false;
                CompilerContext.MessageLog.Error(*FString(TEXT("ContextPin has invalid type: Internal connection error. @@")), this);
            }
            else
            {
                const UEdGraphPin* LinkOwnerPin = OwnerPin->LinkedTo[0];
                const UClass* Own = Cast<UClass>(LinkOwnerPin->PinType.PinSubCategoryObject.Get());
                if (IsValid(Own))
                {

                }
                else
                {
                    bIsErrorFree = false;
                }
                if (bIsErrorFree)
                {
                    CompilerContext.MessageLog.Error(*FString(TEXT("ContextPin has invalid type: Internal connection error. @@")), this);
                }
            }
        }
    }
    */

    SpawnParam.Shrink();

    BreakAllNodeLinks(); // Make sure we caught everything
}

bool UBtf_ExtendConstructObject_K2Node::ConnectSpawnProperties(
    const UClass* ClassToSpawn,
    const UEdGraphSchema_K2* Schema,
    FKismetCompilerContext& CompilerContext,
    UEdGraph* SourceGraph,
    UEdGraphPin*& LastThenPin,
    UEdGraphPin* SpawnedActorReturnPin)
{
    bool bIsErrorFree = true;
    for (const FName OldPinParamName : SpawnParam)
    {
        UEdGraphPin* SpawnVarPin = FindPin(OldPinParamName);

        if (!SpawnVarPin) continue;

        const bool bHasDefaultValue = !SpawnVarPin->DefaultValue.IsEmpty() || !SpawnVarPin->DefaultTextValue.IsEmpty() || SpawnVarPin->DefaultObject;
        if (SpawnVarPin->LinkedTo.Num() > 0 || bHasDefaultValue)
        {
            if (SpawnVarPin->LinkedTo.Num() == 0)
            {
                const FProperty* Property = FindFProperty<FProperty>(ClassToSpawn, SpawnVarPin->PinName);
                if (!Property) continue;

                if (ClassToSpawn->ClassDefaultObject != nullptr)
                {
                    FString DefaultValueAsString;
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

            if (const UFunction* SetByNameFunction = Schema->FindSetVariableByNameFunction(SpawnVarPin->PinType))
            {
                UK2Node_CallFunction* SetVarNode;
                if (SpawnVarPin->PinType.IsArray())
                {
                    SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph);
                }
                else
                {
                    SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
                }
                SetVarNode->SetFromFunction(SetByNameFunction);
                SetVarNode->AllocateDefaultPins();

                // Connect this node into the exec chain
                bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, SetVarNode->GetExecPin());
                if (!bIsErrorFree)
                {
                    CompilerContext.MessageLog.Error(
                        *LOCTEXT("InternalConnectionError", "ExtendConstructObject: Internal connection error. @@").ToString(),
                        this);
                }
                LastThenPin = SetVarNode->GetThenPin();

                static const FName ObjectParamName(TEXT("Object"));
                static const FName ValueParamName(TEXT("Value"));
                static const FName PropertyNameParamName(TEXT("PropertyName"));

                // Connect the new actor to the 'object' pin
                UEdGraphPin* ObjectPin = SetVarNode->FindPinChecked(ObjectParamName);
                SpawnedActorReturnPin->MakeLinkTo(ObjectPin);

                // Fill in literal for 'property name' pin - name of pin is property name
                UEdGraphPin* PropertyNamePin = SetVarNode->FindPinChecked(PropertyNameParamName);
                PropertyNamePin->DefaultValue = SpawnVarPin->PinName.ToString();

                UEdGraphPin* ValuePin = SetVarNode->FindPinChecked(ValueParamName);
                if (SpawnVarPin->LinkedTo.Num() == 0 &&								  //
                    SpawnVarPin->DefaultValue != FString() &&						  //
                    SpawnVarPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte && //
                    SpawnVarPin->PinType.PinSubCategoryObject.IsValid() &&			  //
                    SpawnVarPin->PinType.PinSubCategoryObject->IsA<UEnum>())
                {
                    // Pin is an enum, we need to alias the enum value to an int:
                    UK2Node_EnumLiteral* EnumLiteralNode = CompilerContext.SpawnIntermediateNode<UK2Node_EnumLiteral>(this, SourceGraph);
                    EnumLiteralNode->Enum = CastChecked<UEnum>(SpawnVarPin->PinType.PinSubCategoryObject.Get());
                    EnumLiteralNode->AllocateDefaultPins();
                    EnumLiteralNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(ValuePin);

                    UEdGraphPin* InPin = EnumLiteralNode->FindPinChecked(UK2Node_EnumLiteral::GetEnumInputPinName());
                    InPin->DefaultValue = SpawnVarPin->DefaultValue;
                }
                else
                {
                    // For non-array struct pins that are not linked, transfer the pin type so that the node will expand an auto-ref that will assign the value by-ref.
                    if (SpawnVarPin->PinType.IsArray() == false &&							//
                        SpawnVarPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && //
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
    return bIsErrorFree;
}


bool UBtf_ExtendConstructObject_K2Node::FNodeHelper::ValidDataPin(const UEdGraphPin* Pin, EEdGraphPinDirection Direction)
{
    const bool bValidDataPin = //
        Pin &&				   //
        !Pin->bOrphanedPin &&  //
        (Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec);

    const bool bProperDirection = Pin && (Pin->Direction == Direction);

    return bValidDataPin && bProperDirection;
}

bool UBtf_ExtendConstructObject_K2Node::FNodeHelper::CreateDelegateForNewFunction(
    UEdGraphPin* DelegateInputPin,
    FName FunctionName,
    UK2Node* CurrentNode,
    UEdGraph* SourceGraph,
    FKismetCompilerContext& CompilerContext)
{
    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
    check(DelegateInputPin && Schema && CurrentNode && SourceGraph && (FunctionName != NAME_None));
    bool bResult = true;

    // WORKAROUND, so we can create delegate from nonexistent function by avoiding check at expanding step
    // instead simply: Schema->TryCreateConnection(AddDelegateNode->GetDelegatePin(), CurrentCENode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName));
    UK2Node_Self* SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(CurrentNode, SourceGraph);
    SelfNode->AllocateDefaultPins();

    UK2Node_CreateDelegate* CreateDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CreateDelegate>(CurrentNode, SourceGraph);
    CreateDelegateNode->AllocateDefaultPins();
    bResult &= Schema->TryCreateConnection(DelegateInputPin, CreateDelegateNode->GetDelegateOutPin());
    bResult &= Schema->TryCreateConnection(SelfNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), CreateDelegateNode->GetObjectInPin());
    CreateDelegateNode->SetFunction(FunctionName);

    return bResult;
}

bool UBtf_ExtendConstructObject_K2Node::FNodeHelper::CopyEventSignature(UK2Node_CustomEvent* CENode, UFunction* Function, const UEdGraphSchema_K2* Schema)
{
    check(CENode && Function && Schema);

    bool bResult = true;
    for (TFieldIterator<FProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
    {
        const FProperty* Param = *PropIt;
        if (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
        {
            FEdGraphPinType PinType;
            bResult &= Schema->ConvertPropertyToPinType(Param, PinType);
            bResult &= (nullptr != CENode->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output));
        }
    }
    return bResult;
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
    bool bIsErrorFree = true;
    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
    check(CurrentProperty && ProxyObjectPin && InOutLastThenPin && CurrentNode && SourceGraph && Schema);

    UEdGraphPin* PinForCurrentDelegateProperty = CurrentNode->FindPin(CurrentProperty->GetFName());
    if (!PinForCurrentDelegateProperty || (UEdGraphSchema_K2::PC_Exec != PinForCurrentDelegateProperty->PinType.PinCategory))
    {
        const FText ErrorMessage = FText::Format(
            LOCTEXT("WrongDelegateProperty", "BaseAsyncTask: Cannot find execution pin for delegate "),
            FText::FromString(CurrentProperty->GetName()));
        CompilerContext.MessageLog.Error(*ErrorMessage.ToString(), CurrentNode);
        return false;
    }

	UK2Node_CustomEvent* CurrentCeNode =
		CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(CurrentNode, SourceGraph);
	{
		UK2Node_AddDelegate* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(CurrentNode, SourceGraph);
		AddDelegateNode->SetFromProperty(CurrentProperty, false, CurrentProperty->GetOwnerClass());
		AddDelegateNode->AllocateDefaultPins();
		bIsErrorFree &= Schema->TryCreateConnection(AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), ProxyObjectPin);
		bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
		InOutLastThenPin = AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

        CurrentCeNode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *CurrentProperty->GetName(), *CompilerContext.GetGuid(CurrentNode));
        CurrentCeNode->AllocateDefaultPins();

        bIsErrorFree &= FNodeHelper::CreateDelegateForNewFunction(
            AddDelegateNode->GetDelegatePin(),
            CurrentCeNode->GetFunctionName(),
            CurrentNode,
            SourceGraph,
            CompilerContext);
        bIsErrorFree &= FNodeHelper::CopyEventSignature(CurrentCeNode, AddDelegateNode->GetDelegateSignature(), Schema);
    }

    UEdGraphPin* LastActivatedNodeThen = CurrentCeNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

    const FString DelegateNameStr = CurrentProperty->GetName();
    for (const FNodeHelper::FOutputPinAndLocalVariable& OutputPair : VariableOutputs) // CREATE CHAIN OF ASSIGMENTS
    {
        FString PinNameStr = OutputPair.OutputPin->PinName.ToString();

        if (!PinNameStr.StartsWith(DelegateNameStr))
        {
            continue;
        }

        PinNameStr.RemoveAt(0, DelegateNameStr.Len() + 1);

		UEdGraphPin* PinWithData = CurrentCeNode->FindPinChecked(FName(*PinNameStr));

//++CK
		if (!PinWithData)
		{ return false; }
//--CK

        UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(CurrentNode, SourceGraph);
        AssignNode->AllocateDefaultPins();
        bIsErrorFree &= Schema->TryCreateConnection(LastActivatedNodeThen, AssignNode->GetExecPin());
        bIsErrorFree &= Schema->TryCreateConnection(OutputPair.TempVar->GetVariablePin(), AssignNode->GetVariablePin());
        AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());
        bIsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), PinWithData);
        AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

        LastActivatedNodeThen = AssignNode->GetThenPin();
    }

    bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*PinForCurrentDelegateProperty, *LastActivatedNodeThen).CanSafeConnect();
    return bIsErrorFree;
}

bool UBtf_ExtendConstructObject_K2Node::FNodeHelper::HandleCustomPinsImplementation(
	FMulticastDelegateProperty* CurrentProperty, const TArray<FOutputPinAndLocalVariable>& VariableOutputs,
	UEdGraphPin* ProxyObjectPin, UEdGraphPin*& InOutLastThenPin, UK2Node* CurrentNode, UEdGraph* SourceGraph, TArray<FCustomOutputPin> OutputNames,
	FKismetCompilerContext& CompilerContext)
{
	bool bIsErrorFree = true;
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	check(CurrentProperty && ProxyObjectPin && InOutLastThenPin && CurrentNode && SourceGraph && Schema);

	UK2Node_CustomEvent* CurrentCeNode =
	CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(CurrentNode, SourceGraph);
	{
		UK2Node_AddDelegate* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(CurrentNode, SourceGraph);
		AddDelegateNode->SetFromProperty(CurrentProperty, false, CurrentProperty->GetOwnerClass());
		AddDelegateNode->AllocateDefaultPins();
		bIsErrorFree &= Schema->TryCreateConnection(AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Self), ProxyObjectPin);
		bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
		InOutLastThenPin = AddDelegateNode->FindPinChecked(UEdGraphSchema_K2::PN_Then);

		CurrentCeNode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *CurrentProperty->GetName(), *CompilerContext.GetGuid(CurrentNode));
		CurrentCeNode->AllocateDefaultPins();

		//We need to assign the signature for the proper pins to generate before we connect them
		bIsErrorFree &= FNodeHelper::CreateDelegateForNewFunction(
			AddDelegateNode->GetDelegatePin(),
			CurrentCeNode->GetFunctionName(),
			CurrentNode,
			SourceGraph,
			CompilerContext);
		bIsErrorFree &= FNodeHelper::CopyEventSignature(CurrentCeNode, AddDelegateNode->GetDelegateSignature(), Schema);

		/**Start creating a way for the custom data pin to retrieve the data from the
		 * delegates custom event Data pin.
		 *
		 * This involves creating a temporary variable, then assigning that variable
		 * through the Assign node, which is connected to the custom event's Data pin.
		 *
		 * Varian: Do note, that the debugger will claim the data from the CustomPinData
		 * is correct and valid, BUT the IsValid function for instanced structs
		 * will return false. So we're just one piece away from figuring this out
		 * and until then, this feature does not work. */
		UEdGraphPin* DataPin = CurrentCeNode->FindPin(TEXT("Data"));

		//Create the temporary variable
		UK2Node_TemporaryVariable* TempVarOutput =
			CompilerContext.SpawnInternalVariable(
				CurrentNode,
				DataPin->PinType.PinCategory,
				DataPin->PinType.PinSubCategory,
				DataPin->PinType.PinSubCategoryObject.Get(),
				DataPin->PinType.ContainerType,
				DataPin->PinType.PinValueType
				);


		//Create the assign node and make the appropriate connections
		UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(CurrentNode, SourceGraph);
		AssignNode->AllocateDefaultPins();
		bIsErrorFree &= Schema->TryCreateConnection(CurrentCeNode->FindPinChecked(UEdGraphSchema_K2::PN_Then), AssignNode->GetExecPin());
		bIsErrorFree &= Schema->TryCreateConnection(TempVarOutput->GetVariablePin(), AssignNode->GetVariablePin());
		AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());
		bIsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), DataPin);
		AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

		//This allows the Custom Pin Data to read from the temporary variable
		bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*CurrentNode->FindPin(TEXT("Custom Pin Data")), *TempVarOutput->GetVariablePin()).CanSafeConnect();

		//To simplify the custom output pins, just make a switch node
		UK2Node_SwitchName* SwitchNode = CompilerContext.SpawnIntermediateNode<UK2Node_SwitchName>(CurrentNode, SourceGraph);
		TArray<FName> ConvertedOutputNames;
		for(auto& CurrentPin : OutputNames)
		{
			ConvertedOutputNames.Add(FName(CurrentPin.PinName));
		}
		SwitchNode->PinNames = ConvertedOutputNames;
		SwitchNode->AllocateDefaultPins();

		//Connect the assign node to the switch node
		Schema->TryCreateConnection(AssignNode->GetThenPin(), SwitchNode->GetExecPin());

		//Connect the Custom Event's FName output pin to the Switch's Selection pin
		Schema->TryCreateConnection(CurrentCeNode->FindPin(TEXT("PinName")), SwitchNode->GetSelectionPin());

		//Redirect all the switch nodes output pins to our custom pins
		for (int32 i = 0; i < OutputNames.Num(); ++i)
		{
			const FName& OutputName = FName(OutputNames[i].PinName);
			if(UEdGraphPin* SwitchCasePin = SwitchNode->FindPin(OutputName.ToString()))
			{
				SwitchCasePin->PinToolTip = OutputNames[i].Tooltip;
				UEdGraphPin* NodeOutputPin = CurrentNode->FindPin(OutputName);
				if (SwitchCasePin && NodeOutputPin)
				{
					CompilerContext.MovePinLinksToIntermediate(*NodeOutputPin, *SwitchCasePin);
				}
			}
		}
	}

	return bIsErrorFree;
}

void UBtf_ExtendConstructObject_K2Node::CollectSpawnParam(UClass* InClass, const bool bFullRefresh)
{
    //params
    {
        for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
        {
            const FProperty* Property = *PropertyIt;
            //UClass* PropertyClass = Property->GetOwnerChecked<UClass>();
            const bool bIsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
            const bool bIsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
            const bool bIsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
            const FName PropertyName = Property->GetFName();

            if (!Property->HasAnyPropertyFlags(CPF_Parm) && !bIsDelegate)
            {
                if (bIsExposedToSpawn && bIsSettableExternally && Property->HasAllPropertyFlags(CPF_BlueprintVisible))
                {
                    AllParam.Add(PropertyName);
                    //force add ExposedToSpawn Param
                    SpawnParam.AddUnique(PropertyName);
                }
                else if (
                    !Property->HasAnyPropertyFlags(			 //
                        CPF_NativeAccessSpecifierProtected | //
                        CPF_NativeAccessSpecifierPrivate |	 //
                        CPF_Protected |						 //
                        CPF_BlueprintReadOnly |				 //
                        CPF_EditorOnly |					 //
                        CPF_InstancedReference |			 //
                        CPF_Deprecated |					 //
                        CPF_ExportObject) &&				 //
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
            if (const UFunction* LocFunction = Cast<UFunction>(*It))
            {
                if (LocFunction->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public) &&
                    !LocFunction->HasAnyFunctionFlags(
                        FUNC_Static | FUNC_UbergraphFunction | FUNC_Delegate | FUNC_Private | FUNC_Protected | FUNC_EditorOnly) && // FUNC_BlueprintPure
                    !LocFunction->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) &&
                    !LocFunction->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction) &&
                    !FObjectEditorUtils::IsFunctionHiddenFromClass(LocFunction, InClass) //!InClass->IsFunctionHidden(*LocFunction->GetName())
                )
                {
                    const FName FunctionName = LocFunction->GetFName();

                    const int32 OldNum = AllFunctions.Num();
                    AllFunctions.Add(FunctionName);
                    //force add InitFunction
                    if (LocFunction->GetBoolMetaData(FName("ExposeAutoCall")) || FunctionName.ToString().StartsWith(FNames_Helper::InitPrefix))
                    {
                        AutoCallFunction.AddUnique(FunctionName);
                    }
                    const bool bNewFunction = OldNum - AllFunctions.Num() != 0;
                    if (bNewFunction)
                    {
                        if (FunctionName.ToString().StartsWith(FNames_Helper::InPrefix) && !LocFunction->HasAnyFunctionFlags(FUNC_BlueprintPure | FUNC_Const))
                        {
                            if (bFullRefresh)
                            {
                                ExecFunction.AddUnique(FunctionName);
                            }
                        }
                    }
                }
            }
        }

        if (bFullRefresh)
        {
            Algo::Reverse(ExecFunction);
            Algo::Reverse(AutoCallFunction);
        }
    }

    //delegates
    {
        const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
        const EGraphType GraphType = K2Schema->GetGraphType(GetGraph());
        const bool bAllowOutDelegate = GraphType != EGraphType::GT_Function;

        for (TFieldIterator<FProperty> PropertyIt(InClass); PropertyIt; ++PropertyIt)
        {
            if (const FMulticastDelegateProperty* DelegateProperty = CastField<FMulticastDelegateProperty>(*PropertyIt))
            {
                if (DelegateProperty->HasAnyPropertyFlags(FUNC_Private | CPF_Protected | FUNC_EditorOnly) ||
                    !DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintAssignable) ||
                    DelegateProperty->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) ||
                    DelegateProperty->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction))
                {
                    continue;
                }

                if (const UFunction* LocFunction = DelegateProperty->SignatureFunction)
                {
                    if (!LocFunction->HasAllFunctionFlags(/*FUNC_BlueprintCallable |*/ FUNC_Public) ||
                        LocFunction->HasAnyFunctionFlags(
                            FUNC_Static | FUNC_BlueprintPure | FUNC_Const | FUNC_UbergraphFunction | FUNC_Private | FUNC_Protected | FUNC_EditorOnly) ||
                        LocFunction->GetBoolMetaData(FBlueprintMetadata::MD_BlueprintInternalUseOnly) ||
                        LocFunction->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction))
                    {
                        continue;
                    }
                }

                const FName DelegateName = DelegateProperty->GetFName();
                AllDelegates.Add(DelegateName);

                if (!bFullRefresh)
                {
                    if (OutDelegate.Contains(DelegateName))
                    {
                        if (!bAllowOutDelegate)
                        {
                            OutDelegate.Remove(DelegateName);
                            InDelegate.AddUnique(DelegateName);
                        }
                    }
                }
                else
                {
                    if (DelegateName.ToString().StartsWith(FNames_Helper::InPrefix) ||
                        (DelegateName.ToString().StartsWith(FNames_Helper::OutPrefix) && !bAllowOutDelegate))
                    {
                        InDelegate.AddUnique(DelegateName);
                    }
                    else if (DelegateName.ToString().StartsWith(FNames_Helper::OutPrefix) && bAllowOutDelegate)
                    {
                        OutDelegate.AddUnique(DelegateName);
                    }
                    else
                    {
                        continue;
                    }
                }

                if ((FindPin(DelegateName) != nullptr))
                {
                    bool bAllPinsGood = true;
                    FString DelegateNameStr = DelegateName.ToString();


                    if (DelegateNameStr.StartsWith(FNames_Helper::OutPrefix)) //check Properties
                    {
                        DelegateNameStr.RemoveAt(0, FNames_Helper::OutPrefix.Len());
                        if (const UFunction* DelegateSignatureFunction = DelegateProperty->SignatureFunction)
                        {
                            for (TFieldIterator<FProperty> PropIt(DelegateSignatureFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
                            {
                                const FName ParamName = FName(*FString::Printf(TEXT("%s_%s"), *DelegateNameStr, *(*PropIt)->GetFName().ToString()));
                                bAllPinsGood &= FindPin(ParamName, EGPD_Output) != nullptr;
                            }

                            for (const UEdGraphPin* Pin : Pins)
                            {
                                FString PinNameStr = Pin->GetName();
                                if (PinNameStr.StartsWith(DelegateNameStr))
                                {
                                    PinNameStr.RemoveAt(0, DelegateNameStr.Len() + 1);
                                    bAllPinsGood &= DelegateSignatureFunction->FindPropertyByName(FName(*PinNameStr)) != nullptr;
                                }
                            }
                        }
                    }

                    if (!bAllPinsGood)
                    {
                        if (DelegateNameStr.StartsWith(FNames_Helper::InPrefix))
                        {
                            DelegateNameStr.RemoveAt(0, FNames_Helper::InPrefix.Len());
                        }
                        if (DelegateNameStr.StartsWith(FNames_Helper::OutPrefix))
                        {
                            DelegateNameStr.RemoveAt(0, FNames_Helper::OutPrefix.Len());
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

                        check(FindPin(DelegateName) == nullptr);
                    }
                }
                continue;
            }

            if (const FDelegateProperty* DelegateProperty = CastField<FDelegateProperty>(*PropertyIt))
            {
                check(DelegateProperty);
                //todo not Multicast DelegateProperty
            }
        }
    }


    Algo::Sort(
        AutoCallFunction, //
        [](const FName& A, const FName& B)
        {
            const FString AStr = A.ToString();
            const FString BStr = B.ToString();
            const bool bA_Init = AStr.StartsWith(FNames_Helper::InitPrefix);
            const bool bB_Init = BStr.StartsWith(FNames_Helper::InitPrefix);
            return (bA_Init && !bB_Init) || //
                ((bA_Init && bB_Init) ? UBtf_ExtendConstructObject_Utils::LessSuffix(A, AStr, B, BStr) : A.FastLess(B));
        });
}

#undef LOCTEXT_NAMESPACE