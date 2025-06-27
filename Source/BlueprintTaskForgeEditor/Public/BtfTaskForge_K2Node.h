#pragma once

#include "CoreMinimal.h"
#include "BftMacros.h"

#include "BtfExtendConstructObject_K2Node.h"

#include "BtfTaskForge_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class BLUEPRINTTASKFORGEEDITOR_API UBtf_TaskForge_K2Node : public UBtf_ExtendConstructObject_K2Node
{
    GENERATED_BODY()

public:
    UBtf_TaskForge_K2Node(const FObjectInitializer& ObjectInitializer);

    // UEdGraphNode Interface
    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
    virtual void AllocateDefaultPins() override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    virtual void ReconstructNode() override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetMenuCategory() const override;

    // UK2Node Interface
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

    // Custom Functions
    FString Get_NodeDescription() const;
    FString Get_StatusString() const;
    FLinearColor Get_StatusBackgroundColor() const;
    FText Get_NodeConfigText() const;
    TSet<FName> Get_PinsHiddenByDefault();

#if WITH_EDITORONLY_DATA
    virtual void ResetToDefaultExposeOptions_Impl() override;
#endif

protected:
    void HideClassPin() const;
    void RegisterBlueprintAction(UClass* TargetClass, FBlueprintActionDatabaseRegistrar& ActionRegistrar) const;
    virtual void CollectSpawnParam(UClass* InClass, const bool FullRefresh) override;
};

// --------------------------------------------------------------------------------------------------------------------