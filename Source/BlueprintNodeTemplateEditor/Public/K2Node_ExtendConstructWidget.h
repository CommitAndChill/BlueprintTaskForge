//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_ExtendConstructObject.h"
#include "K2Node_ExtendConstructWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLUEPRINTNODETEMPLATEEDITOR_API UK2Node_ExtendConstructWidget : public UK2Node_ExtendConstructObject
{
	GENERATED_BODY()
public:
	UK2Node_ExtendConstructWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedPtr<class SGraphNode> CreateVisualWidget() override;

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FName GetCornerIcon() const override;
};
