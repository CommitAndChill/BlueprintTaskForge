//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_Blueprint_Template.h"
#include "K2Node_ExtendConstructAiTask.generated.h"

/** */
UCLASS()
class BLUEPRINTNODETEMPLATEEDITOR_API UK2Node_ExtendConstructAiTask : public UK2Node_Blueprint_Template
{
	GENERATED_BODY()
public:
	UK2Node_ExtendConstructAiTask(const FObjectInitializer& ObjectInitializer);

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
#if WITH_EDITORONLY_DATA
	virtual void ResetToDefaultExposeOptions_Impl() override;
#endif
	virtual void CollectSpawnParam(UClass* InClass, const bool bFullRefresh) override;
};
