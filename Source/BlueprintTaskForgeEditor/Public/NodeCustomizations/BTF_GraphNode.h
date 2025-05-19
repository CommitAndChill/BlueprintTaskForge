#pragma once

#include "KismetNodes/SGraphNodeK2Base.h"

#include "NodeDecorators/Btf_NodeDecorator.h"

class UBtf_NodeDecorator;
class UBtf_TaskForge_K2Node;
class UBtf_TaskForge;

class SBtf_Node : public SGraphNodeK2Base
{
public:

	SLATE_BEGIN_ARGS(SBtf_Node) {}
	SLATE_END_ARGS()

	UClass* TaskClass = nullptr;

	TObjectPtr<UBtf_NodeDecorator> Decorator = nullptr;

	virtual ~SBtf_Node()
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
	TWeakObjectPtr<UBtf_TaskForge_K2Node> _BlueprintTaskNode;
	TSharedPtr<STextBlock> _ConfigTextBlock;
};
