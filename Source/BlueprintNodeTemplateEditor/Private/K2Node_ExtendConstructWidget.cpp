//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.


#include "K2Node_ExtendConstructWidget.h"


#include "BlueprintNodeSpawner.h"
#include "ExtendConstructObject_FnLib.h"

#include "SGraphNode.h"
#include "KismetNodes/SGraphNodeK2Default.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "KismetPins/SGraphPinClass.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "K2Node_CallFunction.h"

#define LOCTEXT_NAMESPACE "K2Node"

class SGraphNodeCreateWidget : public SGraphNodeK2Default
{
public:
	// SGraphNode interface
	virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override
	{
		UK2Node_ExtendConstructWidget* CreateWidgetNode = CastChecked<UK2Node_ExtendConstructWidget>(GraphNode);
		UEdGraphPin* ClassPin = CreateWidgetNode->GetClassPin();
		if ((ClassPin == Pin) && (!ClassPin->bHidden || (ClassPin->LinkedTo.Num() > 0)))
		{
			TSharedPtr<SGraphPinClass> NewPin = SNew(SGraphPinClass, ClassPin);
			check(NewPin.IsValid());
			NewPin->SetAllowAbstractClasses(false);
			return NewPin;
		}
		return SGraphNodeK2Default::CreatePinWidget(Pin);
	}
	// End of SGraphNode interface
};


UK2Node_ExtendConstructWidget::UK2Node_ExtendConstructWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UExtendConstructObject_FnLib, ExtendConstructWidget);
	ProxyFactoryClass = UExtendConstructObject_FnLib::StaticClass();
	OutPutObjectPinName = FName(TEXT("Widget"));
	WorldContextPinName = FName(TEXT("WorldContextObject"));
}

TSharedPtr<SGraphNode> UK2Node_ExtendConstructWidget::CreateVisualWidget()
{
	return SNew(SGraphNodeCreateWidget, this);
}

void UK2Node_ExtendConstructWidget::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_ExtendConstructWidget::GetMenuCategory() const
{
	//return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Gameplay);
	UFunction* TargetFunction = GetFactoryFunction();
	return UK2Node_CallFunction::GetDefaultCategoryForFunction(TargetFunction, FText::GetEmpty());
}

FText UK2Node_ExtendConstructWidget::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (ProxyClass)
	{
		const FString Str = ProxyClass->GetName();
		TArray<FString> ParseNames;
		Str.ParseIntoArray(ParseNames, TEXT("_C"));
		return FText::FromString(FString::Printf(TEXT("ExtendConstructWidget_%s"), *ParseNames[0]));
	}
	return FText(LOCTEXT("UK2Node_ExtendConstructWidget", "ExtendConstructWidget"));
}

FName UK2Node_ExtendConstructWidget::GetCornerIcon() const
{
	return TEXT("Graph.Replication.ClientEvent");
}

#undef LOCTEXT_NAMESPACE