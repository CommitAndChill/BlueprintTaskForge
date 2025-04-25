//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_ExtendConstructObject.h"

#include "K2Node_Blueprint_Template.generated.h"

/** */
UCLASS()
class BLUEPRINTNODETEMPLATEEDITOR_API UK2Node_Blueprint_Template : public UK2Node_ExtendConstructObject
{
    GENERATED_BODY()
public:
    UK2Node_Blueprint_Template(const FObjectInitializer& ObjectInitializer);

	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override
	{
		//++V
		/**Note: Without this check, the @ReconstructNode would somehow
		 * lead to a chain of events that would mark the parent blueprint
		 * as dirty as the editor is loading, leading to some blueprints
		 * always being labelled as dirty. This check prevents that. */
		if(Decorator.IsValid())
		{
			/**If the node is using a decorator, we want to refresh the node in case the decorator
			 * is reading the data from the input pins to drive its visuals.
			 * If we instantly reconstruct the node, then we encounter a race condition when we drag
			 * off the pin and create a variable and cause a crash. Using this timer gets around that
			 * that race condition. */
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float DeltaTime) -> bool
			{
				ReconstructNode();
				return false;
			}));
		}
		//--V
	}
	virtual void AllocateDefaultPins() override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;

//++CK
	
	auto Get_NodeDescription() const -> FString;
	auto Get_StatusString() const -> FString;
	auto Get_StatusBackgroundColor() const -> FLinearColor;
	auto Get_NodeConfigText() const -> FText;
	
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