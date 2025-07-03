// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

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