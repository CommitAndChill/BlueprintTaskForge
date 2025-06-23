#pragma once

#include "KismetNodes/SGraphNodeK2Base.h"
#include "BftMacros.h"

#include "NodeDecorators/BtfNodeDecorator.h"

class UBtf_NodeDecorator;
class UBtf_TaskForge_K2Node;
class UBtf_TaskForge;

class SBtf_Node : public SGraphNodeK2Base
{
public:
    SLATE_BEGIN_ARGS(SBtf_Node) {}
    SLATE_END_ARGS()

    virtual ~SBtf_Node()
    {
        if (Decorator)
        {
            Decorator->RemoveFromRoot();
        }
    }

    void Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode, UClass* InTaskClass);

    virtual TSharedRef<SWidget> CreateNodeContentArea() override;
    virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;

    void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) override;

    EVisibility Get_NodeConfigTextVisibility() const;
    FText Get_NodeConfigText() const;

    BFT_PROPERTY_GET(TaskClass)
    BFT_PROPERTY_GET(Decorator)

protected:
    TSharedRef<SWidget> CreateDefaultNodeContent(TSharedPtr<SWidget> TopContent, TSharedPtr<SWidget> CenterContent, TSharedPtr<SWidget> BottomContent);

    TWeakObjectPtr<UBtf_TaskForge_K2Node> BlueprintTaskNode;
    TSharedPtr<STextBlock> ConfigTextBlock;

private:
    UClass* TaskClass = nullptr;
    TObjectPtr<UBtf_NodeDecorator> Decorator = nullptr;
};