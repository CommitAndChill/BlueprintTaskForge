//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_ExtendConstructObject.h"

//++CK
#include "BlueprintTaskTemplate.h"
#include "KismetNodes/SGraphNodeK2Base.h"
//--CK

#include "K2Node_Blueprint_Template.generated.h"

/** */
UCLASS()
class BLUEPRINTNODETEMPLATEEDITOR_API UK2Node_Blueprint_Template : public UK2Node_ExtendConstructObject
{
    GENERATED_BODY()
public:
    UK2Node_Blueprint_Template(const FObjectInitializer& ObjectInitializer);

    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override {}
    virtual void AllocateDefaultPins() override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetMenuCategory() const override;
//++CK
    auto CreateVisualWidget() -> TSharedPtr<SGraphNode> override;
    auto Get_NodeDescription() const -> FString;
    auto Get_StatusString() const -> FString;
    auto Get_StatusBackgroundColor() const -> FLinearColor;
    auto Get_NodeConfigText() const -> FText;

    auto CustomizeProxyFactoryFunction(
        class FKismetCompilerContext& CompilerContext,
        UEdGraph* SourceGraph,
        UK2Node_CallFunction* InCreateProxyObjecNode) -> void override;

    auto
    Get_PinsHiddenByDefault() -> TSet<FName>;
//--CK

#if WITH_EDITORONLY_DATA
    virtual void ResetToDefaultExposeOptions_Impl() override;
#endif

protected:
    void HideClassPin() const;
    void RegisterBlueprintAction(UClass* TargetClass, FBlueprintActionDatabaseRegistrar& ActionRegistrar) const;

    virtual void CollectSpawnParam(UClass* InClass, const bool bFullRefresh) override;
};

// --------------------------------------------------------------------------------------------------------------------

//++CK
class SGraphNode_Blueprint_Template : public SGraphNodeK2Base
{
public:
    SLATE_BEGIN_ARGS(SGraphNode_Blueprint_Template) {}
    SLATE_END_ARGS()

    auto Construct(
        const FArguments& InArgs,
        UK2Node_Blueprint_Template* InNode) -> void;

    auto GetNodeInfoPopups(
        FNodeInfoContext* Context,
        TArray<FGraphInformationPopupInfo>& Popups) const -> void override;

    auto CreateBelowPinControls(
        TSharedPtr<SVerticalBox> MainBox) -> void override;

    auto Get_NodeConfigTextVisibility() const -> EVisibility;
    auto Get_NodeConfigText() const -> FText;

protected:
    TWeakObjectPtr<UK2Node_Blueprint_Template> _BlueprintTaskNode;
    TSharedPtr<STextBlock> _ConfigTextBlock;
};
//--CK