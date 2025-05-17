#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BTF_NodeDecorator.generated.h"

class SBTF_Node;
class UBTF_TaskForge;

/**
 * This class allows you to decorate Blueprint Task nodes.
 * This class does NOT have any runtime logic, nor purpose.
 * It would ideally be editor-only, but we'd get circular dependencies
 * if we wanted to allow nodes to select a decorator in class defaults.
 *
 * This class is C++ only.
 * This class has 4 overridable functions. It tries to provide you a safe
 * and simple way of decorating the top (in between the title and the
 * node content area such as the pins), the center (in between the pins)
 * and the bottom (below the pins).
 *
 * If you really want, you can override the ENTIRE node.
 */
UCLASS(Abstract, NotBlueprintable)
class BLUEPRINTTASKFORGE_API UBTF_NodeDecorator : public UObject
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> CreateTopContent(UClass* TaskClass, UBTF_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode) { return SNullWidget::NullWidget; }

	virtual TSharedRef<SWidget> CreateCenterContent(UClass* TaskClass, UBTF_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode) { return SNullWidget::NullWidget; };

	virtual TSharedRef<SWidget> CreateBottomContent(UClass* TaskClass, UBTF_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode) { return SNullWidget::NullWidget; }

	/**If overriden, you can override the entirety of the node rather than
	 * using the standardized top, center and bottom content.
	 *
	 * Remember to always assign the left and right vertical box, or you'll
	 * get an engine crash.	 */
	virtual TSharedRef<SWidget> OverrideContentNodeArea(SBTF_Node* Node)
	{
		return SNullWidget::NullWidget;
	}

	/**Any objects returned by this function will have a custom details
	 * panel created when the node is selected.
	 * For example, this can be used to edit the class defaults of the task.
	 * Or editing a quest data asset the task node is using to start a quest.
	 *
	 * Tip: This is executed after the "CreateContent" functions. So if you need
	 * data that can only be retrieved from the graph node, you can override
	 * those functions, store the data, then fetch it in this function. */
	virtual TArray<UObject*> GetObjectsForExtraDetailsPanels() const { return TArray<UObject*>(); }
};
