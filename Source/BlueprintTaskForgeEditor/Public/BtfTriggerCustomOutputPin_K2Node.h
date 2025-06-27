#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Engine/MemberReference.h"
#include "BtfTaskForge.h"

#include "BtfTriggerCustomOutputPin_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class FBlueprintActionDatabaseRegistrar;
class UEdGraph;

UCLASS()
class BLUEPRINTTASKFORGEEDITOR_API UBtf_K2Node_TriggerCustomOutputPin : public UK2Node
{
    GENERATED_BODY()

public:
    UBtf_K2Node_TriggerCustomOutputPin(const FObjectInitializer& ObjectInitializer);

    // UEdGraphNode interface
    virtual void AllocateDefaultPins() override;
    virtual FText GetTooltipText() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual bool ShouldShowNodeProperties() const override { return true; }
    virtual void Serialize(FArchive& Ar) override;
    // End of UEdGraphNode interface

    // UK2Node interface
    virtual FText GetMenuCategory() const override;
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
    virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const override;
    virtual bool IsNodePure() const override { return false; }
    // End of UK2Node interface

    // Custom functions
    void RefreshPayloadPins();
    UScriptStruct* GetPayloadStruct() const;
    FCustomOutputPin GetCustomOutputPin() const;
    TArray<FCustomOutputPin> GetAllCustomOutputPins() const;

    // Properties
    UPROPERTY(EditAnywhere, Category = "CustomPin", meta = (GetOptions = "GetCustomPinOptions"))
    FName CustomPinName;

    UPROPERTY()
    UScriptStruct* CachedPayloadType = nullptr;

    UPROPERTY()
    TObjectPtr<UBtf_TaskForge> TaskTemplate = nullptr;

    // Cached custom pins data injected by the compiler extension
    UPROPERTY()
    TArray<FCustomOutputPin> CachedCustomPins;

    // Flag to indicate if cached pins are valid
    UPROPERTY()
    bool bHasValidCachedPins = false;

    // Dropdown support
    UFUNCTION()
    TArray<FString> GetCustomPinOptions() const;

private:
    void CreatePayloadPins();
    bool TryGetCustomPinsFromCache(TArray<FCustomOutputPin>& OutPins) const;
    bool TryGetCustomPinsFromCDO(TArray<FCustomOutputPin>& OutPins) const;

    static const FName PN_TaskForge;
    static const FName PN_CustomPinName;
};

// --------------------------------------------------------------------------------------------------------------------