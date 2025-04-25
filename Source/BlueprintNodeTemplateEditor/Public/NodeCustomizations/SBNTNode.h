#pragma once
#include "KismetNodes/SGraphNodeK2Base.h"
#include "NodeDecorators/BNTNodeDecorator.h"

class UBNTNodeDecorator;
class UK2Node_Blueprint_Template;
class UBlueprintTaskTemplate;

class SBNTNode : public SGraphNodeK2Base
{
public:
	
	SLATE_BEGIN_ARGS(SBNTNode) {}
	SLATE_END_ARGS()

	UClass* TaskClass = nullptr;
	
	TObjectPtr<UBNTNodeDecorator> Decorator = nullptr;

	virtual ~SBNTNode()
	{
		if(Decorator)
		{
			Decorator->RemoveFromRoot();
		}
	}

	void Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode, UClass* InTaskClass);

	virtual TSharedRef<SWidget> CreateNodeContentArea() override;
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;

	auto CreateBelowPinControls(
		TSharedPtr<SVerticalBox> MainBox) -> void override;

	auto Get_NodeConfigTextVisibility() const -> EVisibility;
	auto Get_NodeConfigText() const -> FText;

protected:
	TWeakObjectPtr<UK2Node_Blueprint_Template> _BlueprintTaskNode;
	TSharedPtr<STextBlock> _ConfigTextBlock;
};
