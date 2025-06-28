#include "BtfTriggerCustomOutputPin_K2Node.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "GraphEditorSettings.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_Self.h"
#include "KismetCompiler.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Kismet/BlueprintInstancedStructLibrary.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "K2Node_TriggerCustomOutputPin"

const FName UBtf_K2Node_TriggerCustomOutputPin::PN_TaskForge = TEXT("TaskForge");
const FName UBtf_K2Node_TriggerCustomOutputPin::PN_CustomPinName = TEXT("CustomPinName");

UBtf_K2Node_TriggerCustomOutputPin::UBtf_K2Node_TriggerCustomOutputPin(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UBtf_K2Node_TriggerCustomOutputPin::AllocateDefaultPins()
{
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

    CreatePayloadPins();
    Super::AllocateDefaultPins();
}

FText UBtf_K2Node_TriggerCustomOutputPin::GetTooltipText() const
{
    return LOCTEXT("NodeTooltip", "Triggers a custom output pin on a Blueprint Task Forge node");
}

FText UBtf_K2Node_TriggerCustomOutputPin::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    if (CustomPinName.IsNone())
    {
        return LOCTEXT("NodeTitle_NoPin", "Trigger Custom Output Pin (None Selected)");
    }

    return FText::Format(LOCTEXT("NodeTitle", "Trigger Custom Output Pin: {0}"), FText::FromName(CustomPinName));
}

FSlateIcon UBtf_K2Node_TriggerCustomOutputPin::GetIconAndTint(FLinearColor& OutColor) const
{
    OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
    return FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.AllClasses.FunctionIcon"));
}

void UBtf_K2Node_TriggerCustomOutputPin::PinDefaultValueChanged(UEdGraphPin* Pin)
{
    Super::PinDefaultValueChanged(Pin);
}

void UBtf_K2Node_TriggerCustomOutputPin::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBtf_K2Node_TriggerCustomOutputPin, CustomPinName))
    {
        ReconstructNode();
        GetGraph()->NotifyGraphChanged();
    }
}

void UBtf_K2Node_TriggerCustomOutputPin::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);

    if (Ar.IsLoading() && NOT HasValidCachedPins)
    {
        if (TArray<FCustomOutputPin> OutputPins;
            TryGetCustomPinsFromCDO(OutputPins))
        {
            CachedCustomPins = MoveTemp(OutputPins);
            HasValidCachedPins = true;
        }
    }
}

FText UBtf_K2Node_TriggerCustomOutputPin::GetMenuCategory() const
{
    return LOCTEXT("MenuCategory", "Blueprint Task Forge");
}

void UBtf_K2Node_TriggerCustomOutputPin::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    const auto ActionKey = GetClass();
    if (NOT ActionRegistrar.IsOpenForRegistration(ActionKey))
    { return; }

    if (auto* const NodeSpawner = UBlueprintNodeSpawner::Create(GetClass()))
    {
        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

void UBtf_K2Node_TriggerCustomOutputPin::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    const auto* const Schema = CompilerContext.GetSchema();
    auto* const ExecPin = GetExecPin();
    auto* const ThenPin = FindPinChecked(UEdGraphSchema_K2::PN_Then);

    if (CustomPinName.IsNone() || NOT GetPayloadStruct())
    {
        BreakAllNodeLinks();
        return;
    }

    auto* const SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(this, SourceGraph);
    SelfNode->AllocateDefaultPins();

    auto* const MakeInstancedStructNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    MakeInstancedStructNode->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, MakeInstancedStruct)));
    MakeInstancedStructNode->AllocateDefaultPins();

    auto* const MakeStructNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, SourceGraph);
    MakeStructNode->StructType = GetPayloadStruct();
    MakeStructNode->AllocateDefaultPins();
    MakeStructNode->bMadeAfterOverridePinRemoval = true;

    for (auto* Pin : Pins)
    {
        if (Pin->Direction == EGPD_Input && Pin->PinName != UEdGraphSchema_K2::PN_Execute)
        {
            if (auto* const StructPin = MakeStructNode->FindPin(Pin->PinName))
            {
                CompilerContext.MovePinLinksToIntermediate(*Pin, *StructPin);
            }
        }
    }

    if (auto* const StructOutputPin = MakeStructNode->FindPin(MakeStructNode->StructType->GetFName()))
    {
        if (auto* const ValuePin = MakeInstancedStructNode->FindPin(TEXT("Value")))
        {
            Schema->TryCreateConnection(StructOutputPin, ValuePin);
        }
    }

    auto* const CallFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    CallFunctionNode->FunctionReference.SetExternalMember(
        GET_FUNCTION_NAME_CHECKED(UBtf_TaskForge, TriggerCustomOutputPin),
        UBtf_TaskForge::StaticClass()
    );
    CallFunctionNode->AllocateDefaultPins();

    CompilerContext.MovePinLinksToIntermediate(*ExecPin, *MakeInstancedStructNode->GetExecPin());
    Schema->TryCreateConnection(MakeInstancedStructNode->GetThenPin(), CallFunctionNode->GetExecPin());

    Schema->TryCreateConnection(SelfNode->FindPinChecked(UEdGraphSchema_K2::PN_Self),
                              CallFunctionNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));

    if (auto* const OutputPinPin = CallFunctionNode->FindPin(TEXT("OutputPin")))
    {
        OutputPinPin->DefaultValue = CustomPinName.ToString();
    }

    if (auto* const DataPin = CallFunctionNode->FindPin(TEXT("Data")))
    {
        if (auto* const ResultPin = MakeInstancedStructNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue))
        {
            Schema->TryCreateConnection(ResultPin, DataPin);
        }
    }

    CompilerContext.MovePinLinksToIntermediate(*ThenPin, *CallFunctionNode->GetThenPin());
    BreakAllNodeLinks();
}

void UBtf_K2Node_TriggerCustomOutputPin::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    AllocateDefaultPins();
    RestoreSplitPins(OldPins);

    for (const auto* const OldPin : OldPins)
    {
        if (OldPin->Direction != EGPD_Input)
        { continue; }

        if (auto* const NewPin = FindPin(OldPin->PinName))
        {
            if (NewPin->PinType.PinCategory == OldPin->PinType.PinCategory)
            {
                NewPin->DefaultValue = OldPin->DefaultValue;
                NewPin->DefaultObject = OldPin->DefaultObject;
            }
        }
    }
}

bool UBtf_K2Node_TriggerCustomOutputPin::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
    if (NOT IsValid(TargetGraph))
    { return false; }

    const auto* const Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
    if (NOT IsValid(Blueprint))
    { return false; }

    if (Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(UBtf_TaskForge::StaticClass()))
    {
        return Super::IsCompatibleWithGraph(TargetGraph);
    }

    return false;
}

void UBtf_K2Node_TriggerCustomOutputPin::EarlyValidation(FCompilerResultsLog& MessageLog) const
{
    Super::EarlyValidation(MessageLog);

    if (CustomPinName.IsNone())
    {
        MessageLog.Warning(*LOCTEXT("NoCustomPinSelected", "No custom pin selected in @@").ToString(), this);
    }
}

void UBtf_K2Node_TriggerCustomOutputPin::RefreshPayloadPins()
{
    CreatePayloadPins();
}

TArray<FCustomOutputPin> UBtf_K2Node_TriggerCustomOutputPin::GetAllCustomOutputPins() const
{
    if (TArray<FCustomOutputPin> CustomPins;
        TryGetCustomPinsFromCache(CustomPins) || TryGetCustomPinsFromCDO(CustomPins))
    {
        return CustomPins;
    }

    return TArray<FCustomOutputPin>();
}

bool UBtf_K2Node_TriggerCustomOutputPin::TryGetCustomPinsFromCache(TArray<FCustomOutputPin>& OutPins) const
{
    if (NOT HasValidCachedPins || CachedCustomPins.IsEmpty())
    { return false; }

    OutPins = CachedCustomPins;
    return true;
}

bool UBtf_K2Node_TriggerCustomOutputPin::TryGetCustomPinsFromCDO(TArray<FCustomOutputPin>& OutPins) const
{
    const auto* const Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
    if (NOT IsValid(Blueprint) || NOT Blueprint->GeneratedClass)
    { return false; }

    if (Blueprint->bBeingCompiled)
    { return false; }

    const auto* const TaskCDO = Cast<UBtf_TaskForge>(Blueprint->GeneratedClass->GetDefaultObject());
    if (NOT IsValid(TaskCDO))
    { return false; }

    OutPins = TaskCDO->Get_CustomOutputPins();
    return OutPins.Num() > 0;
}

UScriptStruct* UBtf_K2Node_TriggerCustomOutputPin::GetPayloadStruct() const
{
    const auto CustomPin = GetCustomOutputPin();
    return CustomPin.PayloadType;
}

FCustomOutputPin UBtf_K2Node_TriggerCustomOutputPin::GetCustomOutputPin() const
{
    const auto CustomPins = GetAllCustomOutputPins();

    if (const auto* const FoundPin = Algo::FindBy(CustomPins, CustomPinName, [](const FCustomOutputPin& Pin)
    {
        return FName(Pin.PinName);
    }))
    {
        return *FoundPin;
    }

    return FCustomOutputPin();
}

void UBtf_K2Node_TriggerCustomOutputPin::CreatePayloadPins()
{
    const auto* const PayloadStruct = GetPayloadStruct();
    if (NOT IsValid(PayloadStruct))
    { return; }

    const auto* const K2Schema = GetDefault<UEdGraphSchema_K2>();
    const auto StructSize = PayloadStruct->GetStructureSize();

    auto* const StructData = static_cast<uint8*>(FMemory::Malloc(StructSize));
    PayloadStruct->InitializeStruct(StructData);

    const auto CleanupGuard = [PayloadStruct, StructData]()
    {
        PayloadStruct->DestroyStruct(StructData);
        FMemory::Free(StructData);
    };

    for (TFieldIterator<FProperty> PropertyIterator(PayloadStruct); PropertyIterator; ++PropertyIterator)
    {
        const auto* const Property = *PropertyIterator;

        if (Property->HasAnyPropertyFlags(CPF_Parm) || NOT Property->HasAllPropertyFlags(CPF_BlueprintVisible))
        { continue; }

        FEdGraphPinType PinType;
        if (NOT K2Schema->ConvertPropertyToPinType(Property, PinType))
        { continue; }

        auto* const NewPin = CreatePin(EGPD_Input, PinType, Property->GetFName());
        NewPin->PinFriendlyName = Property->GetDisplayNameText();

        if (K2Schema->PinDefaultValueIsEditable(*NewPin))
        {
            FString DefaultValue;
            FBlueprintEditorUtils::PropertyValueToString(Property, StructData, DefaultValue, this);
            K2Schema->SetPinAutogeneratedDefaultValue(NewPin, DefaultValue);
        }

        K2Schema->ConstructBasicPinTooltip(*NewPin, Property->GetToolTipText(), NewPin->PinToolTip);
    }

    CleanupGuard();
}

TArray<FString> UBtf_K2Node_TriggerCustomOutputPin::GetCustomPinOptions() const
{
    TArray<FString> Options;
    Options.Add(TEXT("None"));

    for (const auto& CustomPins = GetAllCustomOutputPins();
        const auto& Pin : CustomPins)
    {
        Options.Add(Pin.PinName);
    }

    return Options;
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
