#include "SBNTNode.h"

#include "BlueprintTaskTemplate.h"
#include "K2Node_Blueprint_Template.h"
#include "SGraphPanel.h"
#include "Internationalization/BreakIterator.h"
#include "NodeDecorators/BNTNodeDecorator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "K2Node"

void SBNTNode::Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode, UClass* InTaskClass)
{
	GraphNode = InGraphNode;
	TaskClass = InTaskClass;
	_BlueprintTaskNode = Cast<UK2Node_Blueprint_Template>(InGraphNode);
	
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

TSharedRef<SWidget> SBNTNode::CreateNodeContentArea()
{
	TSharedPtr<SWidget> TopContent;
	TSharedPtr<SWidget> CenterContent;
	TSharedPtr<SWidget> BottomContent;

	if(IsValid(_BlueprintTaskNode->GetInstanceOrDefaultObject()->Decorator))
	{
		if(!Decorator)
		{
			/**Make a new instance so things can be more dynamic. Also required for certain
			 * Slate usages, like brushes that need to be stored as a class member. */
			Decorator = NewObject<UBNTNodeDecorator>(_BlueprintTaskNode.Get(), _BlueprintTaskNode->GetInstanceOrDefaultObject()->Decorator);
			_BlueprintTaskNode.Get()->Decorator = Decorator;
			
			/**V: Since decorators are UObject's and Slate REALLY cares about the
			 * lifetime of things, and us storing a pointer to it inside a non-UCLASS
			 * won't prevent it from being garbage collected.
			 * This means that if you hot reload or do anything that causes the
			 * garbage collector to kick in while the decorators elements, like an
			 * image, is being rendered, it will cause a crash.
			 * For now, I am just adding it to root and in the destructor,
			 * we remove it from root. */
			Decorator->AddToRoot();
		}
		
		TSharedPtr<SWidget> NodeOverride = Decorator->OverrideContentNodeArea(this);
		if(NodeOverride != SNullWidget::NullWidget)
		{
			/**The decorator wants to override the node. Return it and
			 * exit out of this function. */
			return NodeOverride.ToSharedRef();
		}
		
		TopContent = Decorator->CreateTopContent(TaskClass, _BlueprintTaskNode.Get()->GetInstanceOrDefaultObject(), _BlueprintTaskNode.IsValid() ? _BlueprintTaskNode.Get() : nullptr);
		CenterContent = Decorator->CreateCenterContent(TaskClass, _BlueprintTaskNode.Get()->GetInstanceOrDefaultObject(), _BlueprintTaskNode.IsValid() ? _BlueprintTaskNode.Get() : nullptr);
		BottomContent = Decorator->CreateBottomContent(TaskClass, _BlueprintTaskNode.Get()->GetInstanceOrDefaultObject(), _BlueprintTaskNode.IsValid() ? _BlueprintTaskNode.Get() : nullptr);
	}
	
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

auto
	SBNTNode::
	GetNodeInfoPopups(
		FNodeInfoContext* Context,
		TArray<FGraphInformationPopupInfo>& Popups) const
	-> void
{
	if(!_BlueprintTaskNode.IsValid())
	{ return; }
	
	if (const auto& Description = _BlueprintTaskNode->Get_NodeDescription();
		Description.IsEmpty() == false)
	{
		// TODO: Expose through developer settings
		// constexpr auto NodeDescriptionBackGroundColor = FLinearColor(0.0625f, 0.0625f, 0.0625f, 1.0f);

		const auto DescriptionPopup = FGraphInformationPopupInfo(nullptr, _BlueprintTaskNode->Get_StatusBackgroundColor(), Description);
		Popups.Add(DescriptionPopup);
	}

	if (GEditor->PlayWorld)
	{
		if (const auto& Status = _BlueprintTaskNode->Get_StatusString();
			Status.IsEmpty() == false)
		{
			const auto DescriptionPopup = FGraphInformationPopupInfo(nullptr, _BlueprintTaskNode->Get_StatusBackgroundColor(), Status);
			Popups.Add(DescriptionPopup);
		}
	}
}

auto
	SBNTNode::
	CreateBelowPinControls(
		TSharedPtr<SVerticalBox> MainBox)
	-> void
{
	static const FMargin ConfigBoxPadding = FMargin(2.0f, 0.0f, 1.0f, 0.0);

	// Add a box to wrap around the Config Text area to make it a more visually distinct part of the node
	TSharedPtr<SVerticalBox> BelowPinsBox;
	MainBox->AddSlot()
		.AutoHeight()
		.Padding(ConfigBoxPadding)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
			.BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 1.0f))
			.Visibility(this, &SBNTNode::Get_NodeConfigTextVisibility)
			[
				SAssignNew(BelowPinsBox, SVerticalBox)
			]
		];

	static const FMargin ConfigTextPadding = FMargin(2.0f, 0.0f, 0.0f, 3.0f);

	BelowPinsBox->AddSlot()
		.AutoHeight()
		.Padding(ConfigTextPadding)
		[
			SAssignNew(_ConfigTextBlock, STextBlock)
			.AutoWrapText(true)
			.LineBreakPolicy(FBreakIterator::CreateWordBreakIterator())
			.Text(this, &SBNTNode::Get_NodeConfigText)
		];
}

auto
	SBNTNode::
	Get_NodeConfigTextVisibility() const
	-> EVisibility
{
	// Hide in lower LODs
	if (const TSharedPtr<SGraphPanel> OwnerPanel = GetOwnerPanel();
		OwnerPanel.IsValid() == false || OwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::MediumDetail)
	{
		if (_ConfigTextBlock.IsValid() && _ConfigTextBlock->GetText().IsEmptyOrWhitespace() == false)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

auto
	SBNTNode::
	Get_NodeConfigText() const
	-> FText
{
	if (IsValid(_BlueprintTaskNode.Get()))
	{ return _BlueprintTaskNode->Get_NodeConfigText(); }

	return FText::GetEmpty();
}

//-CK

#undef LOCTEXT_NAMESPACE

