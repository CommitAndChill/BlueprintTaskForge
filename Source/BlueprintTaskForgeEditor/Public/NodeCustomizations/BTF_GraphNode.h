#pragma once

#include "KismetNodes/SGraphNodeK2Base.h"

#include "NodeDecorators/Btf_NodeDecorator.h"

class UBTF_NodeDecorator;
class UBTF_TaskForge_K2Node;
class UBTF_TaskForge;

class SBTF_Node : public SGraphNodeK2Base
{
public:

	SLATE_BEGIN_ARGS(SBTF_Node) {}
	SLATE_END_ARGS()

	UClass* TaskClass = nullptr;

	TObjectPtr<UBTF_NodeDecorator> Decorator = nullptr;

	virtual ~SBTF_Node()
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
	TWeakObjectPtr<UBTF_TaskForge_K2Node> _BlueprintTaskNode;
	TSharedPtr<STextBlock> _ConfigTextBlock;
};
