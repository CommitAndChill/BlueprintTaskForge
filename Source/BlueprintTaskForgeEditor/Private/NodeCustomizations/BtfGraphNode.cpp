#include "NodeCustomizations/BtfGraphNode.h"

#include "BtfTaskForge.h"
#include "BtfTaskForge_K2Node.h"
#include "SGraphPanel.h"
#include "Internationalization/BreakIterator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "K2Node"

void SBtf_Node::Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode, UClass* InTaskClass)
{
    GraphNode = InGraphNode;
    TaskClass = InTaskClass;
    BlueprintTaskNode = Cast<UBtf_TaskForge_K2Node>(InGraphNode);

    SetCursor(EMouseCursor::CardinalCross);
    UpdateGraphNode();
}

TSharedRef<SWidget> SBtf_Node::CreateNodeContentArea()
{
    auto TopContent = TSharedPtr<SWidget>{};
    auto CenterContent = TSharedPtr<SWidget>{};
    auto BottomContent = TSharedPtr<SWidget>{};

    if (NOT IsValid(BlueprintTaskNode->GetInstanceOrDefaultObject()->Get_Decorator()))
    {
        return CreateDefaultNodeContent(TopContent, CenterContent, BottomContent);
    }

    if (NOT Decorator)
    {
        /**Make a new instance so things can be more dynamic. Also required for certain
         * Slate usages, like brushes that need to be stored as a class member. */
        Decorator = NewObject<UBtf_NodeDecorator>(BlueprintTaskNode.Get(), BlueprintTaskNode->GetInstanceOrDefaultObject()->Get_Decorator());
        BlueprintTaskNode.Get()->Decorator = Decorator;

        /**Since decorators are UObject's and Slate REALLY cares about the
         * lifetime of things, and us storing a pointer to it inside a non-UCLASS
         * won't prevent it from being garbage collected.
         * This means that if you hot reload or do anything that causes the
         * garbage collector to kick in while the decorators elements, like an
         * image, is being rendered, it will cause a crash.
         * For now, I am just adding it to root and in the destructor,
         * we remove it from root. */
        Decorator->AddToRoot();
    }

    if (auto NodeOverride = Decorator->OverrideContentNodeArea(this); 
        NodeOverride != SNullWidget::NullWidget)
    {
        /**The decorator wants to override the node. Return it and
         * exit out of this function. */
        return NodeOverride.ToSharedRef();
    }

    TopContent = Decorator->CreateTopContent(TaskClass, BlueprintTaskNode.Get()->GetInstanceOrDefaultObject(), BlueprintTaskNode.IsValid() ? BlueprintTaskNode.Get() : nullptr);
    CenterContent = Decorator->CreateCenterContent(TaskClass, BlueprintTaskNode.Get()->GetInstanceOrDefaultObject(), BlueprintTaskNode.IsValid() ? BlueprintTaskNode.Get() : nullptr);
    BottomContent = Decorator->CreateBottomContent(TaskClass, BlueprintTaskNode.Get()->GetInstanceOrDefaultObject(), BlueprintTaskNode.IsValid() ? BlueprintTaskNode.Get() : nullptr);

    return CreateDefaultNodeContent(TopContent, CenterContent, BottomContent);
}

TSharedRef<SWidget> SBtf_Node::CreateDefaultNodeContent(TSharedPtr<SWidget> TopContent, TSharedPtr<SWidget> CenterContent, TSharedPtr<SWidget> BottomContent)
{
    return SNew(SVerticalBox)
        //Optional Top Content Slot that takes up the width of the node
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(8, 2)
        .HAlign(HAlign_Fill)
        [
            TopContent.IsValid() ? TopContent.ToSharedRef() : SNullWidget::NullWidget
        ]

        //Main Content (SBorder wrapped)
        + SVerticalBox::Slot()
        .FillHeight(1.0f) //Main content takes up the remaining space
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("NoBorder"))
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            .Padding(FMargin(0, 3))
            [
                SNew(SHorizontalBox)

                //Left Node Box for the left side pins
                + SHorizontalBox::Slot()
                .AutoWidth()
                .HAlign(HAlign_Fill)
                [
                    SAssignNew(LeftNodeBox, SVerticalBox)
                ]

                //Center Content
                + SHorizontalBox::Slot()
                .FillWidth(1.0f) //Center should be space-greedy rather than try to distribute it
                .HAlign(HAlign_Fill)
                [
                    CenterContent.IsValid() ? CenterContent.ToSharedRef() : SNullWidget::NullWidget
                ]

                //Right Node Box for the right side pins
                + SHorizontalBox::Slot()
                .AutoWidth()
                .HAlign(HAlign_Fill)
                [
                    SAssignNew(RightNodeBox, SVerticalBox)
                ]
            ]
        ]

        //Optional Bottom Content Slot that takes up the width of the node
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(8, 2)
        .HAlign(HAlign_Fill)
        [
            BottomContent.IsValid() ? BottomContent.ToSharedRef() : SNullWidget::NullWidget
        ];
}

void SBtf_Node::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
{
    if (NOT BlueprintTaskNode.IsValid()) { return; }

    if (const auto Description = BlueprintTaskNode->Get_NodeDescription(); 
        NOT Description.IsEmpty())
    {
        // TODO: Expose through developer settings
        // constexpr auto NodeDescriptionBackGroundColor = FLinearColor(0.0625f, 0.0625f, 0.0625f, 1.0f);

        const auto DescriptionPopup = FGraphInformationPopupInfo(nullptr, BlueprintTaskNode->Get_StatusBackgroundColor(), Description);
        Popups.Add(DescriptionPopup);
    }

    if (NOT GEditor->PlayWorld) { return; }

    if (const auto Status = BlueprintTaskNode->Get_StatusString(); 
        NOT Status.IsEmpty())
    {
        const auto DescriptionPopup = FGraphInformationPopupInfo(nullptr, BlueprintTaskNode->Get_StatusBackgroundColor(), Status);
        Popups.Add(DescriptionPopup);
    }
}

void SBtf_Node::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox)
{
    static const auto ConfigBoxPadding = FMargin(2.0f, 0.0f, 1.0f, 0.0);

    // Add a box to wrap around the Config Text area to make it a more visually distinct part of the node
    auto BelowPinsBox = TSharedPtr<SVerticalBox>{};
    MainBox->AddSlot()
        .AutoHeight()
        .Padding(ConfigBoxPadding)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
            .BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 1.0f))
            .Visibility(this, &SBtf_Node::Get_NodeConfigTextVisibility)
            [
                SAssignNew(BelowPinsBox, SVerticalBox)
            ]
        ];

    static const auto ConfigTextPadding = FMargin(2.0f, 0.0f, 0.0f, 3.0f);

    BelowPinsBox->AddSlot()
        .AutoHeight()
        .Padding(ConfigTextPadding)
        [
            SAssignNew(ConfigTextBlock, STextBlock)
            .AutoWrapText(true)
            .LineBreakPolicy(FBreakIterator::CreateWordBreakIterator())
            .Text(this, &SBtf_Node::Get_NodeConfigText)
        ];
}

EVisibility SBtf_Node::Get_NodeConfigTextVisibility() const
{
    // Hide in lower LODs
    if (const auto OwnerPanel = GetOwnerPanel(); 
        NOT OwnerPanel.IsValid() || OwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::MediumDetail)
    {
        if (ConfigTextBlock.IsValid() && NOT ConfigTextBlock->GetText().IsEmptyOrWhitespace())
        {
            return EVisibility::Visible;
        }
    }

    return EVisibility::Collapsed;
}

FText SBtf_Node::Get_NodeConfigText() const
{
    if (NOT IsValid(BlueprintTaskNode.Get())) { return FText::GetEmpty(); }

    return BlueprintTaskNode->Get_NodeConfigText();
}

#undef LOCTEXT_NAMESPACE