#pragma once

#include "SGraphPalette.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FBlueprintEditor;

struct FTaskPaletteSummoner
	: public FWorkflowTabFactory
{
	FTaskPaletteSummoner(TSharedPtr<FBlueprintEditor> BlueprintEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

protected:

	TWeakPtr<FBlueprintEditor> WeakBlueprintEditor;
};

/**V: The majority of this palette is butchered together by combining logic
 * from the ActorSequenceEditorTabSummoner and SBlueprintLibraryPalette.
 * I would have made a child of SBlueprintLibraryPalette, but it's in a
 * private folder... yay... */
class SBtf_TaskPalette : public SGraphPalette
{
public:
	SLATE_BEGIN_ARGS(SBtf_TaskPalette) {}
	SLATE_END_ARGS()

	bool IsActiveTimerRegistered = false;

	/** Pointer back to the blueprint editor that owns us */
	TWeakPtr<FBlueprintEditor> BlueprintEditorPtr;

	void Construct(const FArguments& InArgs, TWeakPtr<FBlueprintEditor> InBlueprintEditor);

	UBlueprint* GetBlueprint() const;

	virtual void CollectAllActions(FGraphActionListBuilderBase& OutAllActions) override;

	// SGraphPalette Interface
	virtual void RefreshActionsList(bool bPreserveExpansion) override;
	virtual TSharedRef<SWidget> OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData) override;
	virtual FReply OnActionDragged(const TArray< TSharedPtr<FEdGraphSchemaAction> >& InActions, const FPointerEvent& MouseEvent) override;
	// End of SGraphPalette Interface

	virtual TSharedRef<SVerticalBox> ConstructHeadingWidget(FSlateBrush const* const Icon, FText const& TitleText, FText const& ToolTipText);

	/**Delegate handlers for when the blueprint database is updated (so we can release references and refresh the list)*/
	void OnDatabaseActionsUpdated(UObject* ActionsKey);
	void OnDatabaseActionsRemoved(UObject* ActionsKey);
};
