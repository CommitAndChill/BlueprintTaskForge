﻿// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BtfNodeDecorator.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class SBtf_Node;
class UBtf_TaskForge;

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
class BLUEPRINTTASKFORGE_API UBtf_NodeDecorator : public UObject
{
	GENERATED_BODY()

public:

	virtual TSharedRef<SWidget> CreateTopContent(UClass* TaskClass, UBtf_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode);
	virtual TSharedRef<SWidget> CreateCenterContent(UClass* TaskClass, UBtf_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode);
	virtual TSharedRef<SWidget> CreateBottomContent(UClass* TaskClass, UBtf_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode);

	/**If overriden, you can override the entirety of the node rather than
	 * using the standardized top, center and bottom content.
	 *
	 * Remember to always assign the left and right vertical box, or you'll
	 * get an engine crash.	 */
	virtual TSharedRef<SWidget> OverrideContentNodeArea(SBtf_Node* Node);

	/**Any objects returned by this function will have a custom details
	 * panel created when the node is selected.
	 * For example, this can be used to edit the class defaults of the task.
	 * Or editing a quest data asset the task node is using to start a quest.
	 *
	 * Tip: This is executed after the "CreateContent" functions. So if you need
	 * data that can only be retrieved from the graph node, you can override
	 * those functions, store the data, then fetch it in this function. */
	virtual TArray<UObject*> Get_ObjectsForExtraDetailsPanels() const;
};

// --------------------------------------------------------------------------------------------------------------------