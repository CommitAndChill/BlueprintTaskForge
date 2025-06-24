#include "NodeDecorators/BtfNodeDecorator.h"

// --------------------------------------------------------------------------------------------------------------------

TSharedRef<SWidget> UBtf_NodeDecorator::CreateTopContent(UClass* TaskClass, UBtf_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode)
{
    return SNullWidget::NullWidget;
}

TSharedRef<SWidget> UBtf_NodeDecorator::CreateCenterContent(UClass* TaskClass, UBtf_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode)
{
    return SNullWidget::NullWidget;
}

TSharedRef<SWidget> UBtf_NodeDecorator::CreateBottomContent(UClass* TaskClass, UBtf_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode)
{
    return SNullWidget::NullWidget;
}

TSharedRef<SWidget> UBtf_NodeDecorator::OverrideContentNodeArea(SBtf_Node* Node)
{
    return SNullWidget::NullWidget;
}

TArray<UObject*> UBtf_NodeDecorator::Get_ObjectsForExtraDetailsPanels() const
{
    return TArray<UObject*>();
}

// --------------------------------------------------------------------------------------------------------------------