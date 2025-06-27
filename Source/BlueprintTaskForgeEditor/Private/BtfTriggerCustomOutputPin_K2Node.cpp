#include "BtfTriggerCustomOutputPin_K2Node.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "GraphEditorSettings.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_TemporaryVariable.h"
#include "K2Node_Self.h"
#include "KismetCompiler.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "StructUtils/InstancedStruct.h"
#include "Engine/Blueprint.h"

#include "Kismet/BlueprintInstancedStructLibrary.h"

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

    // Note: We don't create a TaskForge pin because this node should only work on 'self'
    // The expansion will use the self context

    // Create payload pins based on the selected custom pin
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

    // Ensure cached data is serialized
    if (Ar.IsLoading())
    {
        // After loading, if we don't have valid cached pins, try to get them from CDO
        if (!bHasValidCachedPins)
        {
            TArray<FCustomOutputPin> OutputPinsPins;
            if (TryGetCustomPinsFromCDO(OutputPinsPins))
            {
                CachedCustomPins = OutputPinsPins;
                bHasValidCachedPins = true;
            }
        }
    }
}

FText UBtf_K2Node_TriggerCustomOutputPin::GetMenuCategory() const
{
    return LOCTEXT("MenuCategory", "Blueprint Task Forge");
}

void UBtf_K2Node_TriggerCustomOutputPin::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UClass* ActionKey = GetClass();
    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);

        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

void UBtf_K2Node_TriggerCustomOutputPin::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

    // Get our pins
    UEdGraphPin* ExecPin = GetExecPin();
    UEdGraphPin* ThenPin = FindPinChecked(UEdGraphSchema_K2::PN_Then);

    if (!CustomPinName.IsNone() && GetPayloadStruct())
    {
        // Create a self node since we're calling TriggerCustomOutputPin on ourselves
        UK2Node_Self* SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(this, SourceGraph);
        SelfNode->AllocateDefaultPins();

        // Create MakeInstancedStruct node
        UK2Node_CallFunction* MakeInstancedStructNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
        MakeInstancedStructNode->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UBlueprintInstancedStructLibrary, MakeInstancedStruct)));
        MakeInstancedStructNode->AllocateDefaultPins();

        // Create MakeStruct node for the payload
        UK2Node_MakeStruct* MakeStructNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, SourceGraph);
        MakeStructNode->StructType = GetPayloadStruct();
        MakeStructNode->AllocateDefaultPins();
        MakeStructNode->bMadeAfterOverridePinRemoval = true;

        // Connect payload pins to MakeStruct
        for (UEdGraphPin* Pin : Pins)
        {
            if (Pin->Direction == EGPD_Input && Pin->PinName != UEdGraphSchema_K2::PN_Execute)
            {
                UEdGraphPin* StructPin = MakeStructNode->FindPin(Pin->PinName);
                if (StructPin)
                {
                    CompilerContext.MovePinLinksToIntermediate(*Pin, *StructPin);
                }
            }
        }

        // Connect MakeStruct to MakeInstancedStruct
        UEdGraphPin* StructOutputPin = MakeStructNode->FindPin(MakeStructNode->StructType->GetFName());
        UEdGraphPin* ValuePin = MakeInstancedStructNode->FindPin(TEXT("Value"));
        Schema->TryCreateConnection(StructOutputPin, ValuePin);

        // Create the TriggerCustomOutputPin function call
        UK2Node_CallFunction* CallFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
        CallFunctionNode->FunctionReference.SetExternalMember(
            GET_FUNCTION_NAME_CHECKED(UBtf_TaskForge, TriggerCustomOutputPin),
            UBtf_TaskForge::StaticClass()
        );
        CallFunctionNode->AllocateDefaultPins();

        // Connect execution
        CompilerContext.MovePinLinksToIntermediate(*ExecPin, *MakeInstancedStructNode->GetExecPin());
        Schema->TryCreateConnection(MakeInstancedStructNode->GetThenPin(), CallFunctionNode->GetExecPin());

        // Connect self to the function call
        Schema->TryCreateConnection(SelfNode->FindPinChecked(UEdGraphSchema_K2::PN_Self),
                                  CallFunctionNode->FindPinChecked(UEdGraphSchema_K2::PN_Self));

        // Set the custom pin name
        UEdGraphPin* OutputPinPin = CallFunctionNode->FindPin(TEXT("OutputPin"));
        if (OutputPinPin)
        {
            OutputPinPin->DefaultValue = CustomPinName.ToString();
        }

        // Connect the payload
        UEdGraphPin* DataPin = CallFunctionNode->FindPin(TEXT("Data"));
        UEdGraphPin* ResultPin = MakeInstancedStructNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
        Schema->TryCreateConnection(ResultPin, DataPin);

        // Connect output execution
        CompilerContext.MovePinLinksToIntermediate(*ThenPin, *CallFunctionNode->GetThenPin());
    }

    BreakAllNodeLinks();
}

void UBtf_K2Node_TriggerCustomOutputPin::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    AllocateDefaultPins();
    RestoreSplitPins(OldPins);

    for (UEdGraphPin* OldPin : OldPins)
    {
        if (OldPin->Direction == EGPD_Input)
        {
            UEdGraphPin* NewPin = FindPin(OldPin->PinName);
            if (NewPin && NewPin->PinType.PinCategory == OldPin->PinType.PinCategory)
            {
                NewPin->DefaultValue = OldPin->DefaultValue;
                NewPin->DefaultObject = OldPin->DefaultObject;
            }
        }
    }
}

bool UBtf_K2Node_TriggerCustomOutputPin::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
    // This node should only be placeable in BlueprintTaskForge graphs
    if (!TargetGraph)
    {
        return false;
    }

    // Check if the blueprint that owns this graph is a BlueprintTaskForge
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
    if (!Blueprint)
    {
        return false;
    }

    // Check if the generated class is derived from UBtf_TaskForge
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
    TArray<FCustomOutputPin> CustomPins;

    // First try to get from cache
    if (TryGetCustomPinsFromCache(CustomPins))
    {
        return CustomPins;
    }

    // If cache is invalid, try to get from CDO
    if (TryGetCustomPinsFromCDO(CustomPins))
    {
        return CustomPins;
    }

    return TArray<FCustomOutputPin>();
}

bool UBtf_K2Node_TriggerCustomOutputPin::TryGetCustomPinsFromCache(TArray<FCustomOutputPin>& OutPins) const
{
    if (bHasValidCachedPins && CachedCustomPins.Num() > 0)
    {
        OutPins = CachedCustomPins;
        return true;
    }
    return false;
}

bool UBtf_K2Node_TriggerCustomOutputPin::TryGetCustomPinsFromCDO(TArray<FCustomOutputPin>& OutPins) const
{
    // Get the blueprint that owns this node
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
    if (!Blueprint || !Blueprint->GeneratedClass)
    {
        return false;
    }

    // Check if the class is being compiled
    if (Blueprint->bBeingCompiled)
    {
        // During compilation, we can't rely on the CDO
        return false;
    }

    // Get the CDO of the task class
    UBtf_TaskForge* TaskCDO = Cast<UBtf_TaskForge>(Blueprint->GeneratedClass->GetDefaultObject(false));
    if (!TaskCDO)
    {
        return false;
    }

    // Get the custom output pins from the task
    OutPins = TaskCDO->Get_CustomOutputPins();
    return OutPins.Num() > 0;
}

UScriptStruct* UBtf_K2Node_TriggerCustomOutputPin::GetPayloadStruct() const
{
    FCustomOutputPin CustomPin = GetCustomOutputPin();
    return CustomPin.PayloadType;
}

FCustomOutputPin UBtf_K2Node_TriggerCustomOutputPin::GetCustomOutputPin() const
{
    TArray<FCustomOutputPin> CustomPins = GetAllCustomOutputPins();

    // Find the pin with matching name
    for (const FCustomOutputPin& Pin : CustomPins)
    {
        if (FName(Pin.PinName) == CustomPinName)
        {
            return Pin;
        }
    }

    return FCustomOutputPin();
}

void UBtf_K2Node_TriggerCustomOutputPin::CreatePayloadPins()
{
    UScriptStruct* PayloadStruct = GetPayloadStruct();
    if (!PayloadStruct)
    {
        return;
    }

    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

    // Create default instance of the struct to get default values
    uint8* StructData = (uint8*)FMemory::Malloc(PayloadStruct->GetStructureSize());
    PayloadStruct->InitializeStruct(StructData);

    for (TFieldIterator<FProperty> It(PayloadStruct); It; ++It)
    {
        FProperty* Property = *It;

        if (!Property->HasAnyPropertyFlags(CPF_Parm) &&
            Property->HasAllPropertyFlags(CPF_BlueprintVisible))
        {
            FEdGraphPinType PinType;
            if (K2Schema->ConvertPropertyToPinType(Property, PinType))
            {
                UEdGraphPin* NewPin = CreatePin(EGPD_Input, PinType, Property->GetFName());
                NewPin->PinFriendlyName = Property->GetDisplayNameText();

                // Set default value
                if (K2Schema->PinDefaultValueIsEditable(*NewPin))
                {
                    FString DefaultValue;
                    FBlueprintEditorUtils::PropertyValueToString(Property, StructData, DefaultValue, this);
                    K2Schema->SetPinAutogeneratedDefaultValue(NewPin, DefaultValue);
                }

                K2Schema->ConstructBasicPinTooltip(*NewPin, Property->GetToolTipText(), NewPin->PinToolTip);
            }
        }
    }

    // Clean up the temporary struct instance
    PayloadStruct->DestroyStruct(StructData);
    FMemory::Free(StructData);
}

TArray<FString> UBtf_K2Node_TriggerCustomOutputPin::GetCustomPinOptions() const
{
    TArray<FString> Options;

    // Add None option
    Options.Add(TEXT("None"));

    // Get all custom pins
    TArray<FCustomOutputPin> CustomPins = GetAllCustomOutputPins();

    // Add each custom pin name to the options
    for (const FCustomOutputPin& Pin : CustomPins)
    {
        Options.Add(Pin.PinName);
    }

    return Options;
}

#undef LOCTEXT_NAMESPACE